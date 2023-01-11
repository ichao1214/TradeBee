//
// Created by haichao.zhang on 2022/3/29.
//

#include "CTPTraderCallBack.h"
#include "../Message/OrderMessage.h"
#include "../Message/TradeMessage.h"

void CTPTraderCallBack::OnRspOrderInsert(const std::shared_ptr<OrderField>& order) {

    if (order && order->strategy.lock())
    {
        auto msg = std::make_shared<OrderMessage>(IncreaseMsgID(), MessageType::RTNORDER, "ctp_on_rsp_order_insert");
        msg->set_msg_body(order);

        PushMsgToNotifySubscriber(msg, order->strategy.lock());
    }

}

void CTPTraderCallBack::OnRtnOrder(const std::shared_ptr<OrderField>& order) {
//    printf(" CTPTraderCallBack::OnRtnOrder\n");
//    order->print();
    if (order && order->strategy.lock())
    {
        auto msg = std::make_shared<OrderMessage>(IncreaseMsgID(), MessageType::RTNORDER, "ctp_on_rtn_order");
        msg->set_msg_body(order);
        PushMsgToNotifySubscriber(msg, order->strategy.lock());
    }

}

void CTPTraderCallBack::OnRtnTrade(const std::shared_ptr<OrderField>&order, const std::shared_ptr<TradeField>& trade) {
//    printf(" CTPTraderCallBack::OnRtnTrade\n");
//    order->print();
//    trade->print();
    if (order && order->strategy.lock() && trade)
    {
        auto msg = std::make_shared<TradeMessage>(IncreaseMsgID(), MessageType::RTNTRAED, "ctp_on_trade");
        msg->set_msg_body(trade);

        PushMsgToNotifySubscriber(msg, order->strategy.lock());

        auto msg2 = std::make_shared<OrderMessage>(IncreaseMsgID(), MessageType::RTNORDER, "ctp_on_rtn_order");
        msg2->set_msg_body(order);
        PushMsgToNotifySubscriber(msg2, order->strategy.lock());
    }

}

void CTPTraderCallBack::OnRspOrderCancel(const std::shared_ptr<OrderField>& order) {

}

void CTPTraderCallBack::OnRtnOrderAction(const std::shared_ptr<OrderField>& order) {

}
