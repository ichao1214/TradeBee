//
// Created by haichao.zhang on 2022/3/20.
//

#include "M1Strategy.h"
#include "../Monitor/StrategyPositionMonitor.h"


#include <boost/property_tree/xml_parser.hpp>


void M1Strategy::Start() {

//    RegisterTimer(60 * 1000);

//    for (size_t m = 0; m < soha->minute_index_size; ++m)
//    {
//        for (size_t s = 0; s < soha->symbol_index_size; ++s)
//        {
//            auto b = soha->bar_center[m][s];
//            if (b)
//            {
//                printf("m1 get bar: ");
//                b->print();
//            }
//        }
//    }
    StartPullMessage();
    running.store(true);
    trading.store(true);

}

bool M1Strategy::Init() {
    printf("%s [%s] Init %s\n", strategy_name.c_str(), strategy_uuid.c_str(), config.c_str());

    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(config, pt);
    boost::property_tree::ptree symbols = pt.get_child("strategy.symbols");
    boost::property_tree::ptree products = pt.get_child("strategy.products");

    // init default ctp trader
    if (m_traders.find(TraderType::CTP)!= m_traders.end() &&
        m_traders[TraderType::CTP].find("ctp") !=  m_traders[TraderType::CTP].end())
    {
        default_ctp_trader = m_traders[TraderType::CTP].at("ctp");
    }


    // init strategy_symbols
//    for(auto it = symbols.begin(); it != symbols.end(); ++it)
//    {
//        auto symbol_node = it->second;
//        printf("%s append symbol %s\n", strategy_name.c_str(), symbol_node.data().c_str());
//        strategy_symbols.emplace_back(symbol_node.data().c_str());
//    }

    // init strategy_products
    for(auto it = products.begin(); it != products.end(); ++it)
    {
        auto product_node = it->second;
        auto product = product_node.data();
        auto product_symbols = default_ctp_trader->getProductMainSymbol(product);
        auto main_symbol = product_symbols.first;
        printf("%s append product %s, main symbol %s\n", strategy_name.c_str(), product.c_str(), main_symbol.c_str());
        strategy_symbols.emplace_back(main_symbol);
    }

    for (auto m : GlobalData::g_minute_index_map_)
    {
        minute_index_bar_ready_map_[m.second] = false;
    }

    Subscribe(MessageType::CTPTICKDATA, strategy_symbols);
    Subscribe(MessageType::CTPKBARDATA, strategy_symbols);
    Subscribe(MessageType::RTNORDER, strategy_symbols);
    Subscribe(MessageType::RTNTRAED, strategy_symbols);


    return true;
}


void M1Strategy::OnTick(const std::shared_ptr<TICK> &tick) {
//    printf("%s [%s] M1Strategy OnTick: ", strategy_name.c_str(), strategy_uuid.c_str());
//    tick->print();

    auto new_minute = Adj2Min(tick->tick_time);
    size_t new_minute_index = 0;

    try {
        new_minute_index = GlobalData::GetMinuteIndex(new_minute);
    }
    catch (...)
    {
        //printf("get minute %d index fail\n", new_minute);
    }


    if (new_minute_index < 1 )
        return;
    if (minute_index_bar_ready_map_[new_minute_index -1])
        return;

    auto pre_minute_index = new_minute_index - 1;
    if (CheckPreMinuteStrategyBarReady(pre_minute_index))
    {
        printf("strategy bar ready: new minute %d\n", new_minute);

        minute_index_bar_ready_map_[pre_minute_index] = true;

        OnAction(pre_minute_index);

    }

}

bool M1Strategy::CheckPreMinuteStrategyBarReady(size_t & pre_minute_index) {
    bool bar_ready_flag = true;

    for (const auto& symbol : strategy_symbols)
    {
        auto symbol_index = GlobalData::GetSymbolIndex(symbol);
        auto symbol_pre_minute_bar = soha->bar_center[pre_minute_index][symbol_index];

        auto trading_end = GlobalData::g_symbol_trading_end_map_[symbol];
        if (!trading_end
            && symbol_pre_minute_bar == nullptr )
        {
//            printf("%lu %s bar not ready: %d, %x\n",
//                   GlobalData::g_day_miute_vector_[pre_minute_index], symbol.c_str(), trading_end, symbol_pre_minute_bar.get() );
            bar_ready_flag = false;
            break;
        }
    }

    return bar_ready_flag;
}


void M1Strategy::OnNewMinute(int &minute_time_key) {
    printf("%s [%s] OnNewMinute %d\n", strategy_name.c_str(), strategy_uuid.c_str(), minute_time_key);


    auto minute_time_index = GlobalData::g_minute_index_map_[minute_time_key];

    if (minute_time_index > 0)
    {
//        printf("M1Strategy OnNewMinute %d, time index %lu, pre time index %lu\n",
//               minute_time_key, minute_time_index, minute_time_index -1);

//        for (const auto& bar : symbol_bars_of_minute)
//        {
//            if (bar)
//            {
//                bar->print();
//            }
//        }
    }

}

void M1Strategy::OnAction(size_t &pre_minute_time_key_index) {
    printf("%s [%s] OnAction minute index %zu, time %lu\n",
           strategy_name.c_str(), strategy_uuid.c_str(), pre_minute_time_key_index, GlobalData::g_day_miute_vector_[pre_minute_time_key_index]);

    ShowAllPosition();

    ShowPendingOrder();

    if (!CheckTradingTime(pre_minute_time_key_index))
        return;

    if(CancelPendingOrder())
    {
        printf("cancel all pending order success\n");
    }
    else
    {
        printf("cancel all pending order fail\n");
    }

    std::map<double, OrderField> candidate_long;

    std::map<double, OrderField> candidate_short;


    for (const auto& symbol : strategy_symbols)
    {
        double price = 0.0;
        auto symbol_index = GlobalData::GetSymbolIndex(symbol);
        auto symbol_pre_minute_bar = soha->bar_center[pre_minute_time_key_index][symbol_index];

        if (!GlobalData::g_symbol_trading_end_map_[symbol]
            && symbol_pre_minute_bar != nullptr )
        {
            //symbol_pre_minute_bar->print();
            soha->UpdateSymbolTradeCost(symbol_pre_minute_bar->symbol, symbol_pre_minute_bar->close);
            price = symbol_pre_minute_bar->close;
        }



        if (pre_minute_time_key_index >= 9 &&                   //  9.10.11.12...
           !GlobalData::g_symbol_trading_end_map_[symbol])
        {
            int count = 0;
            int need_count = 10;                // 往前取10根bar
            int need_up = 8;
            int need_down = 8;

            int check_need_count = 5;                // 往前取5根bar
            int check_need_up = 4;
            int check_need_down = 4;


            std::vector<double> changes;
            changes.resize(need_count);



            int i = 0;

            int up_count = 0;
            int down_count = 0;
            double total_change = 0.0;
            double avg_rtn = 0.0;
            double total_rtn = 0.0;
            double min_avg_rtn = 0.0005;

            OrderField order{};
            strcpy(order.Symbol, symbol.c_str());
            order.Price = price;

            order.OffsetFlag = OrderOffsetFlag::OPEN;
            order.Volume = GlobalData::GetSymbolField(symbol).MinLimitOrderVolume *3;


            while (count < need_count)                       // 0.1.2....9
            {
                if (pre_minute_time_key_index >= i)          // 防止越界
                {
                    auto bar = soha->bar_center[pre_minute_time_key_index - i][symbol_index];
                    if (bar)
                    {
                        //bar->print();
                        changes[count] = bar->price_change_pct;
                        count ++;
                        //printf("count %d\n", count);
                        bar->price_change_pct > 0 ? up_count++ : bar->price_change_pct < 0 ? down_count++ : 1;
                        total_change += bar->price_change;
                        total_rtn += bar->price_change_pct;

                    }
                    i ++;
                    //printf("loop %s %zu %d %d\n", symbol.c_str(), pre_minute_time_key_index ,count, i);
                }
                else
                {
                    printf("%s bar not enough, %zu, %d, %d\n", symbol.c_str(), pre_minute_time_key_index, count , i );
                    break;
                }

            }
            avg_rtn = total_rtn / need_count;

            if ( up_count >= need_up && total_change >0 && changes[0] > 0 && changes[1] > 0)
            {
                int check_up = 0;
                for (int check = 0; check< check_need_count; ++check)
                {
                    if (changes[check] > 0)
                        check_up++;
                }

                if (check_up >= check_need_up)
                {
                    printf("long: minute %lu %s: up count %d, down_count %d, total change %f, avg rtn %f\n",
                           GlobalData::g_day_miute_vector_[pre_minute_time_key_index], symbol.c_str(), up_count, down_count, total_change, avg_rtn);


                    order.Direction = OrderDirection::BUY;

                    candidate_long.emplace(avg_rtn, order);
                }


            }

            if (down_count >= need_down && total_change < 0 && changes[0] < 0 && changes[1] < 0)
            {
                int check_down = 0;
                for (int check = 0; check< check_need_count; ++check)
                {
                    if (changes[check] < 0)
                        check_down++;
                }

                if (check_down >= check_need_down)
                {
                    printf("short: minute %lu %s: up count %d, down_count %d, total change %f, avg rtn %f\n",
                           GlobalData::g_day_miute_vector_[pre_minute_time_key_index], symbol.c_str(), up_count, down_count, total_change, avg_rtn);

                    order.Direction = OrderDirection::SELL;

                    candidate_short.emplace(avg_rtn, order);
                }
            }

        }

    }
    printf("candidate long size %lu\n", candidate_long.size());
    printf("candidate short size %lu\n", candidate_short.size());

    size_t topN = 5;
    // send order

    // long order
    auto ls = candidate_long.size();
    auto long_topN = topN;
    if (ls < long_topN )
        long_topN = ls;
    std::map<double, OrderField> real_long(candidate_long.rbegin(), std::next(candidate_long.rbegin(), long_topN));


    printf("\n");
    printf("real long order top %lu\n", long_topN);
    printf("++++++++++++++++++++ %10s Long Start ++++++++++++++++++++\n", strategy_name.c_str());
    for (auto it = real_long.rbegin(); it!=real_long.rend(); it++ )
    {
        printf("avg rtn: %f ", it->first);
        it->second.print();

        auto p = GetSymbolPosition(it->second.Symbol);

        if (p && p->LongTotal >= it->second.Volume)
        {
            printf("%s not need send, current position ", it->second.Symbol);
            p->print();
            printf("\n");
            continue;
        }

        auto new_order_id = Buy(it->second.Symbol, it->second.Price, it->second.Volume);
        printf("new strategy order id: %d\n", new_order_id);
        printf("\n");

    }
    printf("++++++++++++++++++++ %10s Long End  ++++++++++++++++++++\n", strategy_name.c_str());


    // short order
    auto ss = candidate_short.size();
    auto short_topN = topN;

    if (ss < short_topN )
        short_topN = ss;

    std::map<double, OrderField> real_short(candidate_short.begin(), std::next(candidate_short.begin(), short_topN));


    printf("\n");
    printf("real short order top %lu\n", short_topN);
    printf("-------------------- %10s Short Start --------------------\n", strategy_name.c_str());
    for (auto & o : real_short)
    {
        printf("avg rtn: %f ", o.first);
        o.second.print();
        auto p = GetSymbolPosition(o.second.Symbol);

        if (p && p->ShortTotal >= o.second.Volume)
        {
            printf("%s not need send, current position ", o.second.Symbol);
            p->print();
            printf("\n");
            continue;
        }

        auto new_order_id = Short(o.second.Symbol, o.second.Price, o.second.Volume);
        printf("new strategy order id: %d\n", new_order_id);
        printf("\n");
    }
    printf("-------------------- %10s Short End  --------------------\n", strategy_name.c_str());
}

bool M1Strategy::CheckTradingTime(const size_t &pre_minute_time_key_index) {

    // 14:59 或者 22:59 后 打印日内未平仓单
    if (pre_minute_time_key_index >= GlobalData::GetMinuteIndex(145800)
    || pre_minute_time_key_index >= GlobalData::GetMinuteIndex(225800) && pre_minute_time_key_index <= GlobalData::GetMinuteIndex(230000))
    {
        trading.store(false);
        running.store(false);
        printf("now time %lu, show not closed position\n", GlobalData::g_day_miute_vector_[pre_minute_time_key_index]);

        ShowAllPosition();
        return false;
    }

    // 14:56 或者 22：56 后 平掉日内仓位，停止下单
    if (pre_minute_time_key_index >= GlobalData::GetMinuteIndex(145500)
        || pre_minute_time_key_index >= GlobalData::GetMinuteIndex(225500) && pre_minute_time_key_index <= GlobalData::GetMinuteIndex(230000))
    {
        trading.store(false);
        printf("now time %lu, stop trading\n", GlobalData::g_day_miute_vector_[pre_minute_time_key_index]);
        if (CancelPendingOrder())
        {
            printf("cancel all pending open order success\n");
        }
        else
        {
            printf("cancel all pending open order fail\n");
        }

        ShowAllPosition();
        printf("stop monitor\n");
        strategyPositionMonitor->Stop();

        printf("close all position\n");
        CloseAllPosition();
        return false;
    }

    running.store(true);
    trading.store(true);
    strategyPositionMonitor->Start();

    return true;
}



void M1Strategy::OnBar(const std::shared_ptr<BAR> &bar) {
//    printf("M1Strategy OnBar: ");
//    bar->print();


//    std::shared_ptr<OrderField> new_order = nullptr;
//    if ( (bar->average - bar->average_price_daily) < 0)
//    {
//        new_order = default_ctp_trader->Buy(bar->symbol, bar->close, 1, this->GetSharedPtr());
//    }
//    else if ((bar->average - bar->average_price_daily) > 0)
//    {
//        new_order = default_ctp_trader->Short(bar->symbol, bar->close, 1, this->GetSharedPtr());
//    }
//
//    if (new_order)
//    {
//        auto strategy_order_id = IncreaseStrategyOrderID();
//        AppendStrategyOrder(strategy_order_id, new_order);
//        UpdateOrderIDMap(new_order->OrderID, strategy_order_id);
//    }
}

void M1Strategy::OnRtnOrder(const std::shared_ptr<OrderField> &order) {
    printf("%s [%s] OnRtnOrder: \n",strategy_name.c_str(), strategy_uuid.c_str());
    if (order->Status == OrderStatus::CANCELED)
    {
        printf("order canceled\n");
    }
    order->print();
    if (order->OffsetFlag != OrderOffsetFlag::OPEN && order->Status== OrderStatus::CANCELED && trading.load())   // 平仓撤单
    {
        auto close_strategy_order_id = GetStrategyOrderID(order->OrderID);
        strategyPositionMonitor->AppendStrategyTradeByCloseID(close_strategy_order_id);
    }
}

void M1Strategy::OnRtnTrade(const std::shared_ptr<TradeField> &trade) {
    printf("%s [%s] OnRtnTrade: \n", strategy_name.c_str(), strategy_uuid.c_str());

    printf("new trade: ");
    trade->print();

    auto order = GetStrategyOrderByTraderOrderID(trade->OrderID);
    if (trade->OffsetFlag == OrderOffsetFlag::OPEN && trading.load())
    {
        strategyPositionMonitor->AppendStrategyTrade(order, trade);
    }


}


void M1Strategy::OnTimer() {
    printf("%s [%s] OnTimer\n", strategy_name.c_str(), strategy_uuid.c_str());
    if(CancelPendingOrder())
    {
        printf("cancel all pend order success\n");
    }
    else
    {
        printf("cancel all pend order fail\n");
    }
}










