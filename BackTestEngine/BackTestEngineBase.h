//
// Created by haichao.zhang on 2022/3/31.
//

#ifndef TRADEBEE_BACKTESTENGINEBASE_H
#define TRADEBEE_BACKTESTENGINEBASE_H

#include <thread>
#include <mutex>
#include <queue>
#include "../DataStruct/TradeBeeDataStruct.h"
#include <atomic>
#include <unordered_map>
#include <utility>
#include "../TraderBase/TraderBase.h"
#include "BackTestTraderSpi.h"
#include "RingBuffer.h"
#include "boost/lockfree/spsc_queue.hpp"

class BackTestEngineBase : public EventSubscriberBase{

private:
    std::mutex raw_order_queue_mtx_;
    std::list<std::shared_ptr<OrderField>> raw_order_queue_{};
    std::mutex raw_cancel_request_queue_mtx_;
    std::list<std::shared_ptr<OrderField>> raw_cancel_request_queue_{};
    std::mutex accepted_order_map_mtx_;
    std::mutex trade_queue_mtx_;
    std::queue<std::shared_ptr<TradeField>> trade_queue_{};



//    std::shared_ptr<RingBuffer<std::shared_ptr<OrderField>>> raw_order_ring_;
//    std::shared_ptr<RingBuffer<std::shared_ptr<OrderField>>> raw_cancel_ring_;


    boost::lockfree::spsc_queue<std::shared_ptr<OrderField>, boost::lockfree::capacity<1024*1024>> raw_order_ring_;
    boost::lockfree::spsc_queue<std::shared_ptr<OrderField>, boost::lockfree::capacity<1024*1024>> raw_cancel_ring_;

    std::vector<std::shared_ptr<OrderField>> accepted_order_vec_;





    std::atomic<bool> running{};

    int internal_order_sys_id{};
    int internal_trade_id{};
    std::shared_ptr<std::thread> deal_order;
    std::shared_ptr<std::thread> deal_cancel;
    std::shared_ptr<std::thread> rtn_trade;



    std::weak_ptr<BackTestTraderSpi> trade_spi;

protected:
    std::unordered_map<std::string, TICK> symbol_order_book{};

    std::mutex symbol_matching_order_list_mtx_;
    std::unordered_map<std::string, std::list<std::shared_ptr<OrderField>>> symbol_matching_order_list_;   // 待撮合订单

public:
    BackTestEngineBase()
    {
        //printf("constructor BackTestEngineBase\n");
        symbol_order_book.clear();
        running.store(false);



        accepted_order_vec_.resize(1024*1024);
        std::fill(accepted_order_vec_.begin(),accepted_order_vec_.end(), nullptr);

    }

    ~BackTestEngineBase()
    {
        printf("~BackTestEngineBase dtor\n");
    }

    virtual void Init()
    {
        //StartTask();
    }

    void RegisterSpi(std::weak_ptr<BackTestTraderSpi> traderSpi)
    {
        trade_spi = std::move(traderSpi);
    }


    void DealOrder()
    {
        printf("start deal order thread\n");
        while (running.load())
        {
            std::shared_ptr<OrderField> order;
            if (raw_order_ring_.pop(order) && order != nullptr)
            {
                printf("backtest engine deal order ");
                order->print();
                PreRtnOrder(order);
                trade_spi.lock()->OnBackTestRtnOrder(order);
            }
        }
    }

    void DealCancel()
    {
        printf("start deal cancel thread\n");
        while (running.load())
        {
            std::shared_ptr<OrderField> order;

            if (raw_cancel_ring_.pop(order) && order != nullptr)
            {
                auto cancel = RemoveMatchingOrder(order);
                if(cancel)
                {
                    printf("backtest engine cancel order success: ");
                    cancel->print();
                    trade_spi.lock()->OnBackTestRtnOrder(cancel);
                }
                else
                {
                    printf("backtest engine cancel order fail: ");
                    order->print();
                }
            }
        }
    }


    void ReqInsertOrder(std::shared_ptr<OrderField>& order)
    {
        while (!raw_order_ring_.push(order));

        printf("backtest ReqInsertOrder: ");
        order->print();
    }


    bool ReqCancelOrder(const std::string& exchange_id, std::string order_sys_id, int order_id, std::string symbol)
    {
        if (order_sys_id.empty())
            return false;
        auto original_order = GetOrderByOrderRef(order_sys_id);
        if (original_order)
        {
            printf("backtest ReqCancelOrder original order:");
            original_order->print();

            while (!raw_cancel_ring_.push(original_order));

        }
        return true;
    }


//    void AppendTrade(std::shared_ptr<TradeField>& trade)
//    {
//
//        printf("backtest append new trade: ");
//        trade->print();
//    }


    void RtnTrade(std::shared_ptr<TradeField> &trade, std::shared_ptr<OrderField> &order)
    {
        if (trade)
        {
            printf("backtest new trade: ");
            trade->print();

            if (order && (order->Status == OrderStatus::ALL_TRADED || order->Status == OrderStatus::PART_TRADED))
            {
                trade_spi.lock()->OnBackTestRtnOrder(order);
                trade_spi.lock()->OnBackTestRtnTrade(order, trade);
            }
        }
    }

    void RtnOrder(std::shared_ptr<OrderField> &order)
    {
        if (order)
        {
            printf("backtest rtn order: ");
            order->print();

            if (order)
            {
                trade_spi.lock()->OnBackTestRtnOrder(order);
            }
        }
    }

    int IncreaseOrderSysID()
    {
        return ++internal_order_sys_id;
    }


    int IncreaseTradeID()
    {
        return ++internal_trade_id;
    }


    std::shared_ptr<OrderField> GetOrderByOrderRef(std::string order_ref)
    {
        return accepted_order_vec_[std::stoi(order_ref)];

    }

    void FeedTick(std::shared_ptr<TICK> & new_tick)
    {
        symbol_order_book[new_tick->symbol] = *new_tick;
        MatchOrder(new_tick);
    }

    TICK GetLastOrderBook(const std::string& symbol) const
    {
        return symbol_order_book.at(symbol);
    }

    virtual void PreRtnOrder(std::shared_ptr<OrderField>& order)
    {

        order->Status = OrderStatus::ACCEPTED;
        auto last_tick = GetLastOrderBook(order->Symbol);
        bool long_ = (order->Direction == OrderDirection::BUY);  // 多头开仓  空头平仓

        bool short_ = (order->Direction == OrderDirection::SELL);  // 多头平仓 空头开仓

        if (long_ && order->Price - last_tick.upper_limit_price > 1e-5                 // 多头涨停
           || short_ && order->Price - last_tick.lower_limit_price < 1e-5)             // 空头跌停
        {
            order->Status = OrderStatus::REJECTED;
        }

        strncpy(order->InsertTime, last_tick.update_time, sizeof(TimeType)-1 );
        strncpy(order->UpdateTime, last_tick.update_time, sizeof(TimeType)-1 );


        if (order->Volume < GlobalData::GetSymbolField(order->Symbol).MinLimitOrderVolume
        || order->Volume > GlobalData::GetSymbolField(order->Symbol).MaxLimitOrderVolume)
        {
            order->Status = OrderStatus::REJECTED;
        }

        auto order_ref = IncreaseOrderSysID();

        strcpy(order->OrderRef, std::to_string(order_ref).c_str());

        accepted_order_vec_[order_ref] = order;

        if (order->Status == OrderStatus::ACCEPTED)
        {
            strncpy(order->AcceptedTime, last_tick.update_time, sizeof(TimeType)-1);

            printf("PushPreMatchingOrder: ");
            order->print();
            std::lock_guard<std::mutex> l(symbol_matching_order_list_mtx_);
            auto it = symbol_matching_order_list_.find(order->Symbol);
            if (it != symbol_matching_order_list_.end())
            {
                it->second.push_back(order);
            }
            else
            {
                std::list<std::shared_ptr<OrderField>> ol{order};
                symbol_matching_order_list_.emplace(order->Symbol, ol);
            }
        }
    }


    std::shared_ptr<OrderField> RemoveMatchingOrder(std::shared_ptr<OrderField> &original_order)
    {
        std::shared_ptr<OrderField> cancel_order = nullptr;
        std::lock_guard<std::mutex> l(symbol_matching_order_list_mtx_);
        auto it = symbol_matching_order_list_.find(original_order->Symbol);
        if (it != symbol_matching_order_list_.end())
        {
            auto & order_list = it->second;
            auto it2 = order_list.begin();
            while (it2!=order_list.end())
            {
                if ((*it2)->OrderID == original_order->OrderID)
                {
                    cancel_order = (*it2);
                    cancel_order->Status = OrderStatus::CANCELED;
                    order_list.erase(it2);

                    break;
                }
                else
                {
                    ++it2;
                }
            }
            if (it2==order_list.end())
            {
                printf("RemoveMatchingOrder: order not find: ");
                original_order->print();
            }

        }
        else
        {
            printf("RemoveMatchingOrder: symbol not find: ");
            original_order->print();
        }
        return cancel_order;
    }


    virtual void MatchOrder(std::shared_ptr<TICK>& tick) = 0;

    void OnPushMessage(std::shared_ptr<MessageBase> msg)
    {
        auto msg_type = msg->get_msg_type();

        switch(msg_type)
        {
            case MessageType::CTPTICKDATA:
            {
                auto ctp_tick_data = std::make_shared<TICK>(msg->get_msg_body().tick);
//                printf("backtest engine OnTick: ");
//                ctp_tick_data->print();
                FeedTick(ctp_tick_data);

                break;
            }
            default:
                printf("msg %d not support", msg_type);
                break;
        }
    }

//    void OnNoticeMessage(MessageType type)
//    {
//
//    }

public:
    void StartMatchTaskThread()
    {
        if (trade_spi.lock() == nullptr)
        {
            printf("start task fail, trade spi nullptr\n");
        }
        running.store(true);
        deal_order = std::make_shared<std::thread>(&BackTestEngineBase::DealOrder, this);
        deal_cancel = std::make_shared<std::thread>(&BackTestEngineBase::DealCancel, this);
        //rtn_trade = std::make_shared<std::thread>(&BackTestEngineBase::RtnTrade, this);
    }


    void JoinMatchTaskThread()
    {
        deal_order->join();
        deal_cancel->join();
        //rtn_trade->join();
    }

    void StopTask()
    {
        running.store(false);

    }


};


#endif //TRADEBEE_BACKTESTENGINEBASE_H
