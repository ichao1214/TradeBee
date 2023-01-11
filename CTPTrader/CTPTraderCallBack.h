//
// Created by haichao.zhang on 2022/3/29.
//

#ifndef TRADEBEE_CTPTRADERCALLBACK_H
#define TRADEBEE_CTPTRADERCALLBACK_H

#include "../TraderBase/TraderCallBackBase.h"

class CTPTraderCallBack : public  TraderCallBackBase{

public:
    void OnRspOrderInsert(const std::shared_ptr<OrderField>& order) override;
    void OnRtnOrder(const std::shared_ptr<OrderField>& order) override;
    void OnRtnTrade(const std::shared_ptr<OrderField>&order, const std::shared_ptr<TradeField>& trade) override;
    void OnRspOrderCancel(const std::shared_ptr<OrderField>& order) override;
    void OnRtnOrderAction(const std::shared_ptr<OrderField>& order) override;
};


#endif //TRADEBEE_CTPTRADERCALLBACK_H
