//
// Created by haichao.zhang on 2022/4/13.
//

#include "StrategyPositionMonitor.h"
#include <boost/property_tree/xml_parser.hpp>


bool StrategyPositionMonitor::Init() {

    printf("%s Init %s\n", strategy_name.c_str(), config.c_str());


    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(config, pt);
    boost::property_tree::ptree common = pt.get_child("monitor.common");
    boost::property_tree::ptree products = pt.get_child("monitor.products");

    common_stop_order_paras_->stop_price_tick = common.get<double>("<xmlattr>.stop_price_tick");
    common_stop_order_paras_->trail_price_tick = common.get<double>("<xmlattr>.trail_price_tick");

    printf("common stop price tick %f, common trail price tick %f\n",
           common_stop_order_paras_->stop_price_tick,  common_stop_order_paras_->trail_price_tick );

    for (auto it = products.begin(); it != products.end(); ++it)
    {
        if (it->first == "product")
        {
            const auto & product_id = it->second.get<std::string>("<xmlattr>.id");
            const auto & stop_price_tick = it->second.get<double>("<xmlattr>.stop_price_tick");
            const auto & trail_price_tick = it->second.get<double>("<xmlattr>.trail_price_tick");
            auto st = std::make_shared<StopOrderPara>(stop_price_tick, trail_price_tick);

            products_stop_order_paras_.emplace(product_id, st);

            printf("product id: %s, stop price tick %f, trail price tick %f\n",
                   product_id.c_str(), st->stop_price_tick, st->trail_price_tick);
        }
    }


    for (const auto& s : GlobalData::g_symbol_map_) {
        strategy_symbols.push_back(s.first);
    }

    Subscribe(MessageType::CTPTICKDATA, strategy_symbols);
    return true;
}

void StrategyPositionMonitor::Start() {

    printf("%s start\n", strategy_name.c_str());
    running.store(true);
}

void StrategyPositionMonitor::OnTick(const std::shared_ptr<TICK> &tick) {

    if (running.load() == false) return;

    int close_strategy_order_id = -1;
    std::lock_guard<std::mutex> l(sots_mtx_);
    auto it = symbols_order_trades_map_.find(tick->symbol);
    if (it != symbols_order_trades_map_.end())
    {
        auto & sots = it->second;
        auto it2 = sots.begin();
        while ( it2!=sots.end())
        {
            auto order = (*it2)->getOrder();
            auto trade = (*it2)->getTrade();

            auto stop_price = (*it2)->getStopPrice();
            auto stop_type =(*it2)->getSt();

            if ((*it2)->CheckStop(tick))
            {
                printf("need send stop order: %s, order id %d, order ref %s, trade id %s, direction: %s, trade price %.3lf, stop price %.3lf, volume %d\n",
                       order->Symbol, order->OrderID, order->OrderRef, trade->TradeID, order->Direction==OrderDirection::BUY? "Open":"Sell",
                       trade->TradePrice, stop_price, trade->TradeVolume);

                double price = stop_price;
                if (order->Direction == OrderDirection::BUY)
                {
                    if (stop_type == StopType::STP)
                    {
                        price = tick->bid_price[0];
                    }
                    close_strategy_order_id = order->strategy.lock()->Sell(order->Symbol, price, order->Volume);
                    printf("sell %s, price %f, volume %d,  close strategy order id %d\n",order->Symbol, price, order->Volume, close_strategy_order_id);
                }
                else
                {
                    if (stop_type == StopType::STP)
                    {
                        price = tick->ask_price[0];
                    }
                    close_strategy_order_id = order->strategy.lock()->Cover(order->Symbol, price, order->Volume);
                    printf("cover %s, price %f, volume %d, close strategy order id %d\n", order->Symbol, price, order->Volume, close_strategy_order_id);
                }

                if (close_strategy_order_id != -1)
                {
                    printf("send close success\n");

                    auto close_order = order->strategy.lock()->GetStrategyOrderByStrategyID(close_strategy_order_id);
                    close_order_map_[close_strategy_order_id] = *it2;
                    it2 = sots.erase(it2);
                }
                else
                {
                    it2++;
                    printf("send close fail, continue send close when next tick : %s %d %f %x\n",
                           order->Symbol, order->Direction, stop_price, order->strategy.lock().get());
                }


            }
            else
            {
                it2++;
                //printf("send close fail, continue send close when next tick : %s %d %f %x\n", symbol.c_str(), direction, stop_price, strategy.get());
            }




        }
    }


}
