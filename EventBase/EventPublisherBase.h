//
// Created by haichao.zhang on 2021/12/30.
//

#ifndef TRADEBEE_EVENTPUBLISHERBASE_H
#define TRADEBEE_EVENTPUBLISHERBASE_H

#include <list>
#include <vector>
#include <algorithm>
#include <memory>
#include "EventSubscriberBase.h"



#include <mutex>
#include <utility>
#include <cstring>


class EventPublisherBase {
private:
    std::list<std::weak_ptr<EventSubscriberBase>> subscribers_;
    std::mutex sub_mtx_;

protected:
    std::vector<std::string> notice_symbols;

public:
    EventPublisherBase()
    {

    }

    ~EventPublisherBase()
    {
        printf("~EventPublisherBase dtor\n");
        subscribers_.erase(subscribers_.begin(),subscribers_.end());
    }


    virtual bool RegisterSubscriber(const std::weak_ptr<EventSubscriberBase>& subscriber)
    {
        std::lock_guard<std::mutex> lock(sub_mtx_);
        subscribers_.push_back(subscriber);

        if (subscriber.lock() == nullptr)
            printf("sub null ptr\n");
        //subscriber->AppendMsgArray(pub_msgs_);

        return true;
    }

    void RemoveSubscriber(const std::weak_ptr<EventSubscriberBase>& subscriber)
    {
        std::lock_guard<std::mutex> lock(sub_mtx_);
        subscribers_.erase(remove_if(subscribers_.begin(), subscribers_.end(),
                                     [subscriber](const std::weak_ptr<EventSubscriberBase>& sub)
                                     { return subscriber.lock() == sub.lock();}), subscribers_.end());

    }

    // push all
    void PushOrNoticeMsgToAllSubscriber(const std::shared_ptr<MessageBase>& msg)
    {
        //MessageBase messageBase {};
        //messageBase = *msg;

        std::lock_guard<std::mutex> lock(sub_mtx_);
        for (const auto& sub : subscribers_)
        {
            PushMsgToNotifySubscriber(msg, sub.lock());
        }
    }

    // push one
    static void PushMsgToNotifySubscriber(const std::shared_ptr<MessageBase>& msg, const std::shared_ptr<EventSubscriberBase>& sub)
    {
        if (sub == nullptr) printf("sub nullptr\n");
        if (sub->GetSubscribeMode() == SubscribeMode::PUSH)
        {
            sub->PushMessage(msg);
        }
        else if (sub->GetSubscribeMode() == SubscribeMode::PULL)
        {
//            if (msg->get_msg_type() == MessageType::CTPKBARDATA)
//                printf("push bar\n");
            sub->msgs_queue_->push(msg);
        }
    }




    virtual bool Init() {return true;};
    virtual bool Login(std::string username, std::string password, char** paras = nullptr) {return true;};
};


#endif //TRADEBEE_EVENTPUBLISHERBASE_H
