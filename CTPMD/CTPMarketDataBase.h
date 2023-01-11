//
// Created by haichao.zhang on 2022/4/8.
//

#ifndef TRADEBEE_CTPMARKETDATABASE_H
#define TRADEBEE_CTPMARKETDATABASE_H

#include "../EventBase/EventPublisherBase.h"

class CTPMarketDataBase : public  EventPublisherBase {

public:
    virtual void SubscribeCTPMD(MessageType type, std::vector<std::string> symbols) = 0;
    virtual void StartPushMarketDataThread() {};
    virtual void JoinPushMarketDataThread() {};
};


#endif //TRADEBEE_CTPMARKETDATABASE_H
