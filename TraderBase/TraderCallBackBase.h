//
// Created by zhanghaichao on 2021/12/17.
//

#ifndef TRADEBEE_TRADERCALLBACKBASE_H
#define TRADEBEE_TRADERCALLBACKBASE_H

#include "../DataStruct/TradeBeeDataStruct.h"
#include "../EventBase/EventPublisherBase.h"
#include "../StrategyBase/StrategyBase.h"

class TraderCallBackBase: public EventPublisherBase {

public:
    virtual void OnRspOrderInsert(const  std::shared_ptr<OrderField>& order)=0;
    virtual void OnRtnOrder(const std::shared_ptr<OrderField>& order) = 0;
    virtual void OnRtnTrade(const std::shared_ptr<OrderField>&order, const std::shared_ptr<TradeField>& trade) = 0;
    virtual void OnRspOrderCancel(const std::shared_ptr<OrderField>&order) = 0;
    virtual void OnRtnOrderAction(const std::shared_ptr<OrderField>& order) = 0;

    void RegisterStrategy(const std::shared_ptr<StrategyBase>& strategy)
    {

    }

    int IncreaseMsgID()
    {
        std::unique_lock<std::mutex> ul(mtx);
        return ++callback_msg_id;
    }

protected:
    std::mutex mtx;
    int callback_msg_id;

};



#endif //TRADEBEE_TRADERCALLBACKBASE_H
