//
// Created by haichao.zhang on 2021/12/30.
//

#ifndef TRADEBEE_EVENTSUBSCRIBERBASE_H
#define TRADEBEE_EVENTSUBSCRIBERBASE_H

#include "../DataStruct/TradeBeeDataStruct.h"
#include "../Message/MessageBase.h"
#include "CircularArray.h"
#include <memory>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <unordered_set>


enum SubscribeMode
{
    PUSH=0x01,
    PULL=0x02,
};

class EventPublisherBase;


class EventSubscriberBase {
private:

    SubscribeMode subscribeMode;

    std::shared_ptr<std::thread> pull_thread_;

    std::atomic<bool> running;

    std::shared_ptr<CircularArray<MessageBase>> msgs_queue_;

    size_t msgs_queue_cap_;

    std::mutex subscribe_topic_symbol_map_mtx_;
    std::unordered_map<MessageType, std::string, EnumClassHash> subscribe_topic_symbol_map_;



protected:
    //std::vector<std::shared_ptr<CircularArray<MessageBase>>> sub_msgs_vec_;


public:
//    EventSubscriberBase()
//    {
//        //printf("constructor EventSubscriberBase\n");
//        subscribeMode = SubscribeMode::PUSH;
//        running.store(false);
//    }

    friend class EventPublisherBase;

    EventSubscriberBase(SubscribeMode mode = SubscribeMode::PUSH, size_t msgs_queue_cap_ = 65535)
    :subscribeMode(mode), msgs_queue_cap_(msgs_queue_cap_)
    {
        msgs_queue_ = std::make_shared<CircularArray<MessageBase>>(msgs_queue_cap_);
        printf("constructor EventSubscriberBase, mode %d, msg queue size %lu\n", mode, msgs_queue_cap_);
        subscribe_topic_symbol_map_.clear();
        running.store(false);
    }

    void StartPullMessage() {

        if (subscribeMode == SubscribeMode::PULL)
        {
            running.store(true);
            printf("start pull msg\n");
            pull_thread_ = std::make_shared<std::thread>(&EventSubscriberBase::PullMessage, this);
        }
    }

    ~EventSubscriberBase()
    {

        printf("~EventSubscriberBase\n");
    }

    SubscribeMode GetSubscribeMode() {return subscribeMode;}

    virtual void OnPushMessage(std::shared_ptr<MessageBase> msg) = 0;

//    virtual void OnNoticeMessage(MessageType type) = 0;

    void PullMessage()
    {
        while (running.load())
        {
            auto msg = msgs_queue_->pop();
            if (FilterMsg(msg))
                OnPushMessage(msg);
        }
    }

    void PushMessage(std::shared_ptr<MessageBase> msg)
    {
        if (FilterMsg(msg))
            OnPushMessage(msg);
    }

//    void NoticeMessage(MessageType type)
//    {
//        OnNoticeMessage(type);
//    }

    void Subscribe(MessageType type, const std::string& symbol)
    {
        std::lock_guard<std::mutex> l(subscribe_topic_symbol_map_mtx_);
        auto it = subscribe_topic_symbol_map_.find(type);
        if (it != subscribe_topic_symbol_map_.end())
        {
            auto & src = it->second;
            if(strstr(src.c_str(), symbol.c_str()) == nullptr)
                src.append(symbol);
            //printf("S 1 %d %s %d %s\n", type, symbol.c_str(), it->first, it->second.c_str());
        }
        else
        {
            subscribe_topic_symbol_map_.emplace(type, symbol);
            //printf("S 2 %d %s\n", type, symbol.c_str());
        }
    }


    void Subscribe(MessageType type, const std::vector<std::string>& symbols)
    {
        for (const auto& s : symbols)
        {
            Subscribe(type, s);
        }
    }

    inline bool FilterMsg(const std::shared_ptr<MessageBase>& msg)
    {
        auto msg_type = msg->get_msg_type();
        std::lock_guard<std::mutex> l(subscribe_topic_symbol_map_mtx_);

        auto it = subscribe_topic_symbol_map_.find(msg_type);
        //printf("F 0 %d %lu\n", msg_type, subscribe_topic_symbol_map_.size());

        if (it == subscribe_topic_symbol_map_.end())
        {
            return false;
        }

        auto subs_symbols = it->second;
        std::string msg_symbol;
        switch (msg_type) {

            case MessageType::CTPTICKDATA:
            {
                msg_symbol  = std::string(msg->get_msg_body().tick.symbol);
                break;
            }
            case MessageType::CTPKBARDATA:
            {

                msg_symbol = std::string(msg->get_msg_body().bar.symbol);
                //printf("filter bar msg %s\n", msg_symbol.c_str());
                break;
            }
            case MessageType::RTNORDER:
            {
                //printf("MessageType::RTNORDER\n");
                msg_symbol = std::string(msg->get_msg_body().order.Symbol);
                break;
            }
            case MessageType::RTNTRAED:
            {
                msg_symbol = std::string(msg->get_msg_body().trade.Symbol);
                break;
            }
            case MessageType::STOCKKBARDATA:
                break;
            default:
                printf("msg %d not support", msg_type);
                break;

        }

        if (msg_symbol.empty() || strstr(subs_symbols.c_str(), msg_symbol.c_str()) == nullptr)
            return false;

        return true;

    }

    void JoinPullThread()
    {
        if (subscribeMode == PULL && pull_thread_)
        {

            pull_thread_->join();

        }


    }

//    void AppendMsgArray( std::shared_ptr<CircularArray<MessageBase>> msgs_array)
//    {
//        sub_msgs_vec_.emplace_back(msgs_array);
//    }
};


#endif //TRADEBEE_EVENTSUBSCRIBERBASE_H
