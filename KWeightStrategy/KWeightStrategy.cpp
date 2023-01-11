//
// Created by haichao.zhang on 2022/4/19.
//

#include "KWeightStrategy.h"

#include <memory>
#include <set>


bool KWeightStrategy::Init() {
    M1Strategy::Init();
//    strategyParas = std::make_unique<StrategyParas>();
//
//    // k1 涨跌幅权重
//    strategyParas->kpa[0].length_=need_bar_count;
//    strategyParas->kpa[0].max_value_=1.00;
//    strategyParas->kpa[0].min_value_=0.00;
//    strategyParas->kpa[0].precision_=2;
//    strategyParas->kpd[0] = new double [strategyParas->kpa[0].length_];

    // k2 成交量占比权重
//    strategyParas->kpa[1].length_=need_bar_count;
//    strategyParas->kpa[1].max_value_=1.00;
//    strategyParas->kpa[1].min_value_=0.00;
//    strategyParas->kpa[1].precision_=2;
//    strategyParas->kpd[1] = new double [strategyParas->kpa[1].length_];

    return true;
}

void KWeightStrategy::OnAction(size_t &pre_minute_time_key_index) {

    auto pre_time_ =GlobalData::g_day_miute_vector_[pre_minute_time_key_index];
    printf("%s [%s] OnAction minute index %zu, time %lu\n",
           strategy_name.c_str(), strategy_uuid.c_str(), pre_minute_time_key_index, pre_time_);

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

            std::vector<double> changes_pct;    // 涨跌幅
            std::vector<double> close;          // 收盘价
            std::vector<int> volume;            // 成交量
            std::vector<int> price_amplitude;   // 震幅



            double w1 = 0.0;

            changes_pct.resize(need_bar_count);
            close.resize(need_bar_count);
            volume.resize(need_bar_count);


            int total_volume = 0;

            int i = 0;

            OrderField order{};
            strcpy(order.Symbol, symbol.c_str());
            order.Price = price;

            order.OffsetFlag = OrderOffsetFlag::OPEN;
            order.Volume = GlobalData::GetSymbolField(symbol).MinLimitOrderVolume;


            while (count < need_bar_count)                       // 0.1.2....9
            {
                if (pre_minute_time_key_index >= i)          // 防止越界
                {
                    auto bar = soha->bar_center[pre_minute_time_key_index - i][symbol_index];
                    if (bar)
                    {
                        //bar->print();
                        changes_pct[count] = bar->price_change_pct;
                        close[count] = bar->close;
                        volume[count] = bar->volume;
                        total_volume += bar->volume;
                        count ++;

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

            // data ready
            for(int j = 0; j < need_bar_count; ++j)               // 前 j 根 bar
            {
                auto vk = strategyParas.kpd[0][j] * (volume[j] / (double )total_volume);   // 成交量占比 权重
                auto pk = strategyParas.kpd[0][j] * changes_pct[j] ;                       // 涨跌幅权重
                w1 += vk + pk;
            }



            printf("%s %lu %f\n", symbol.c_str(), pre_time_, w1);

            if (w1 > 0)
            {
                order.Direction = OrderDirection::BUY;

                candidate_long.emplace(w1, order);
            }
            else if (w1 < 0)
            {
                order.Direction = OrderDirection::SELL;

                candidate_short.emplace(w1, order);
            }

        }

    }

    printf("\n");
    printf("candidate long size %lu\n", candidate_long.size());
    printf("candidate short size %lu\n", candidate_short.size());


    // send order

    // long order
    auto ls = candidate_long.size();
    auto long_topN = topN;
    if (ls < long_topN )
        long_topN = ls;
    std::map<double, OrderField> real_long(candidate_long.rbegin(), std::next(candidate_long.rbegin(), long_topN));

    std::set<std::string> adj_symbol;
    printf("\n");
    printf("real long order top %lu\n", long_topN);
    printf("++++++++++++++++++++ %10s Long Start ++++++++++++++++++++\n", strategy_name.c_str());
    for (auto it = real_long.rbegin(); it!=real_long.rend(); it++ )
    {
        printf("Long: w1 %f, symbol %s, price %.3lf, volume %d\n",
               it->first, it->second.Symbol,  it->second.Price,it->second.Volume);
        adj_symbol.emplace(it->second.Symbol);
        SetTargetPosition(it->second.Symbol, it->second.Price, it->second.Volume);

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
    for (auto it = real_short.begin(); it!=real_short.end() ; ++it)
    {
        printf("Short: w1 %f, symbol %s, price %.3lf, volume %d\n",
               it->first, it->second.Symbol,it->second.Price, it->second.Volume);
        adj_symbol.emplace(it->second.Symbol);
        SetTargetPosition(it->second.Symbol, it->second.Price, -it->second.Volume);
    }
    printf("-------------------- %10s Short End  --------------------\n", strategy_name.c_str());

    // 其他合约平仓

    for (const auto& s: strategy_symbols)
    {
        if (adj_symbol.find(s) == adj_symbol.end())
        {
            SetTargetPosition(s,symbols_last_ticks[s]->last_price ,0);
        }
    }



}

void KWeightStrategy::OnRtnOrder(const std::shared_ptr<OrderField> &order) {

    printf("%s [%s] OnRtnOrder: \n",strategy_name.c_str(), strategy_uuid.c_str());
    if (order->Status == OrderStatus::CANCELED)
    {
        printf("order canceled\n");
    }
    order->print();
    if (order->OffsetFlag != OrderOffsetFlag::OPEN && order->Status== OrderStatus::CANCELED && trading.load())   // 平仓撤单
    {
        printf("close order canceled\n");
        auto price_tick = GlobalData::GetSymbolField(order->Symbol).PriceTick;
        double price=0;
        if (order->Direction == OrderDirection::BUY)
        {
            price = symbols_last_ticks[order->Symbol]->ask_price[0] +  price_tick * 3;
            Cover(order->Symbol, price , order->Volume);
            printf("resend cover: %s, price %.3lf, volume %d\n", order->Symbol, price, order->Volume);
        }
        else
        {
            price = symbols_last_ticks[order->Symbol]->bid_price[0] - price_tick * 3;
            Sell(order->Symbol, price , order->Volume);
            printf("resend sell: %s, price %.3lf, volume %d\n", order->Symbol, price, order->Volume);
        }

    }

}

void KWeightStrategy::OnRtnTrade(const std::shared_ptr<TradeField> &trade) {
    printf("%s [%s] OnRtnTrade: \n", strategy_name.c_str(), strategy_uuid.c_str());

    printf("new trade: ");
    trade->print();
}



