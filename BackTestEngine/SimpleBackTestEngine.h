//
// Created by haichao.zhang on 2022/3/31.
//

#ifndef TRADEBEE_SIMPLEBACKTESTENGINE_H
#define TRADEBEE_SIMPLEBACKTESTENGINE_H
#include "BackTestEngineBase.h"

class SimpleBackTestEngine : public BackTestEngineBase {
private:
    std::unordered_map<std::string, std::list<std::shared_ptr<OrderField>>> order_queue_map_;

public:

    SimpleBackTestEngine()
    {
        printf("constructor SimpleBackTestEngine\n");
    }

    void Init() override
    {
        std::vector<std::string> symbols;
        for (const auto& s: GlobalData::g_symbol_map_)
        {
            symbols.push_back(s.first);
        }
        Subscribe(MessageType::CTPTICKDATA, symbols);
    }

    void MatchOrder(std::shared_ptr<TICK>& tick) override
    {
        // adjust tick
        if (tick->ask_price[1] < 1e-5)
        {
            for (int i = 1;i<10;++i)
            {
                tick->ask_price[i] = tick->ask_price[i-1] + GlobalData::GetSymbolField(tick->symbol).PriceTick;
                if(tick->ask_price[i] - tick->upper_limit_price > 0.00005)   // 涨停
                    break;
                tick->ask_volume[i] = tick->ask_volume[i-1];
            }
        }

        if (tick->bid_price[1]  < 1e-5)
        {
            for (int i = 1;i<10;++i)
            {
                tick->bid_price[i] = tick->bid_price[i-1] - GlobalData::GetSymbolField(tick->symbol).PriceTick;
                if(tick->bid_price[i] - tick->lower_limit_price < 0.00005)   // 跌停
                    break;
                tick->bid_volume[i] = tick->bid_volume[i-1];
            }
        }
//        printf("tick after adjust: ");
//        tick->print();

        std::lock_guard<std::mutex> l(symbol_matching_order_list_mtx_);
        auto it = symbol_matching_order_list_.find(tick->symbol);
        if (it != symbol_matching_order_list_.end())
        {
            auto & order_list = it->second;
            {
                auto it_order = order_list.begin();
                while (it_order != order_list.end())
                {
                    auto order = (*it_order);


                    if (order->Direction == OrderDirection::BUY)          // 多头开仓  空头平仓
                    {
                        if (order->Price >= tick->ask_price[0])         // 买单价格大于等于卖一
                        {
                            auto trade = std::make_shared<TradeField>();
                            trade->OrderID = order->OrderID;
                            strcpy(trade->Symbol, order->Symbol);
                            strcpy(trade->OrderRef, order->OrderRef);
                            auto trade_id = IncreaseTradeID();
                            strcpy(trade->TradeID, std::to_string(trade_id).c_str());
                            trade->Direction = order->Direction;
                            trade->OffsetFlag = order->OffsetFlag;
                            trade->TradePrice = tick->ask_price[0];
                            strncpy(trade->TradeTime, tick->update_time, sizeof(TimeType)-1);

                            auto not_trade =  order->Volume - order->VolumeTraded;
                            not_trade = not_trade - tick->ask_volume[0];
                            if (not_trade <= 0)                                   // 剩余全成
                            {
                                order->Status = OrderStatus::ALL_TRADED;
                                trade->TradeVolume = order->Volume - order->VolumeTraded;
                                order->VolumeTraded = trade->TradeVolume;
                                strncpy(order->UpdateTime, tick->update_time, sizeof(TimeType)-1);

                                RtnTrade(trade, order);
                                it_order = order_list.erase(it_order);
                                continue;
                            }
                            else                                             // 部分成交
                            {
                                order->Status = OrderStatus::PART_TRADED;
                                trade->TradeVolume = tick->ask_volume[0];
                                order->VolumeTraded = trade->TradeVolume;
                                strncpy(order->UpdateTime, tick->update_time, sizeof(TimeType)-1);

                                RtnTrade(trade, order);

                                if (order->Price >= tick->ask_price[1])         // 剩余订单以卖二全部成交
                                {
                                    auto trade2 = std::make_shared<TradeField>();
                                    trade2->OrderID = order->OrderID;
                                    strcpy(trade2->Symbol, order->Symbol);
                                    strcpy(trade2->OrderRef, order->OrderRef);

                                    auto trade_id2 = IncreaseTradeID();
                                    strcpy(trade2->TradeID, std::to_string(trade_id2).c_str());

                                    trade2->Direction = order->Direction;
                                    trade2->OffsetFlag = order->OffsetFlag;
                                    strncpy(trade2->TradeTime, tick->update_time, sizeof(TimeType)-1);

                                    order->Status = OrderStatus::ALL_TRADED;
                                    trade2->TradeVolume = not_trade;
                                    trade2->TradePrice = tick->ask_price[1];
                                    order->VolumeTraded = order->VolumeTraded + not_trade;
                                    strncpy(order->UpdateTime, tick->update_time, sizeof(TimeType)-1);

                                    RtnTrade(trade2, order);
                                    it_order = order_list.erase(it_order);
                                    continue;
                                }
                                else                                           // 买单价格小于等于卖二，继续挂着
                                {
                                    it_order ++;
                                }
                            }
                            tick->ask_volume[0] =                              // 更新盘口
                                    tick->ask_volume[0] - order->Volume > 0 ? tick->ask_volume[0] - order->Volume : 0;

                        }
                        else                                                 // 买单价格小于等于卖一，继续挂着
                        {
                            it_order ++;
                        }
                    }
                    else                                                  // 空头
                    {
                        if (order->Price <= tick->bid_price[0])         // 卖单价格笑于等于买一
                        {
                            auto trade = std::make_shared<TradeField>();
                            trade->OrderID = order->OrderID;
                            strcpy(trade->Symbol, order->Symbol);
                            strcpy(trade->OrderRef, order->OrderRef);

                            auto trade_id = IncreaseTradeID();
                            strcpy(trade->TradeID, std::to_string(trade_id).c_str());

                            trade->Direction = order->Direction;
                            trade->OffsetFlag = order->OffsetFlag;
                            trade->TradePrice = tick->bid_price[0];
                            strncpy(trade->TradeTime, tick->update_time, sizeof(TimeType)-1 );

                            auto not_trade =  order->Volume - order->VolumeTraded;
                            not_trade = not_trade - tick->bid_volume[0];
                            if (not_trade <= 0)                                   // 剩余全成
                            {
                                order->Status = OrderStatus::ALL_TRADED;
                                trade->TradeVolume = order->Volume - order->VolumeTraded;
                                order->VolumeTraded =  trade->TradeVolume;
                                strncpy(order->UpdateTime, tick->update_time,  sizeof(TimeType)-1 );

                                RtnTrade(trade, order);
                                it_order = order_list.erase(it_order);
                                continue;
                            }
                            else                                             // 部分成交
                            {
                                order->Status = OrderStatus::PART_TRADED;
                                trade->TradeVolume = tick->bid_volume[0];
                                order->VolumeTraded = trade->TradeVolume;
                                strncpy(order->UpdateTime, tick->update_time, sizeof(TimeType)-1 );

                                RtnTrade(trade, order);

                                if (order->Price <= tick->bid_price[1])         // 剩余订单以买二全部成交
                                {
                                    auto trade2 = std::make_shared<TradeField>();
                                    trade2->OrderID = order->OrderID;
                                    strcpy(trade2->Symbol, order->Symbol);
                                    strcpy(trade2->OrderRef, order->OrderRef);

                                    auto trade_id2 = IncreaseTradeID();
                                    strcpy(trade2->TradeID, std::to_string(trade_id2).c_str());

                                    trade2->Direction = order->Direction;
                                    trade2->OffsetFlag = order->OffsetFlag;
                                    strncpy(trade2->TradeTime, tick->update_time, sizeof(TimeType)-1);

                                    order->Status = OrderStatus::ALL_TRADED;
                                    trade2->TradeVolume = not_trade;
                                    trade2->TradePrice = tick->bid_price[1];
                                    order->VolumeTraded = order->VolumeTraded + not_trade;
                                    strncpy(order->UpdateTime, tick->update_time, sizeof(TimeType)-1);

                                    RtnTrade(trade2, order);
                                    it_order = order_list.erase(it_order);
                                    continue;
                                }
                                else                                           // 买单价格小于等于卖二，继续挂着
                                {
                                    it_order ++;
                                }
                            }
                            tick->bid_volume[0] =                              // 更新盘口
                                    tick->bid_volume[0] - order->Volume > 0 ? tick->bid_volume[0] - order->Volume : 0;

                        }
                        else                                                 // 买单价格小于等于卖一，继续挂着
                        {
                            it_order ++;
                        }
                    }
                }
            }
        }

    }



};


#endif //TRADEBEE_SIMPLEBACKTESTENGINE_H
