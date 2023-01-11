//
// Created by haichao.zhang on 2022/3/31.
//

#ifndef TRADEBEE_ORDERMESSAGE_H
#define TRADEBEE_ORDERMESSAGE_H
#include "MessageBase.h"

class OrderMessage : public MessageBase {

public:

    OrderMessage(int id, MessageType type, std::string remark)
            : MessageBase(id, type, std::move(remark)) {

    }




    void set_msg_body(const std::shared_ptr<OrderField>& order)
    {
        msg_body.order.OrderID = order->OrderID;

        strcpy(msg_body.order.Symbol, order->Symbol);

        msg_body.order.Direction = order->Direction;
        msg_body.order.OffsetFlag = order->OffsetFlag;
        msg_body.order.Volume = order->Volume;
        msg_body.order.Price = order->Price;
        strcpy(msg_body.order.OrderRef, order->OrderRef);
        msg_body.order.VolumeTraded = order->VolumeTraded;
        strcpy(msg_body.order.InsertTime, order->InsertTime);
        strcpy(msg_body.order.AcceptedTime, order->AcceptedTime);
        strcpy(msg_body.order.UpdateTime, order->UpdateTime);
        msg_body.order.Status = order->Status;
        msg_body.order.strategy = order->strategy;
    }


};


#endif //TRADEBEE_ORDERMESSAGE_H
