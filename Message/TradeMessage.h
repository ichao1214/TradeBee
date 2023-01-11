//
// Created by haichao.zhang on 2022/3/31.
//

#ifndef TRADEBEE_TRADEMESSAGE_H
#define TRADEBEE_TRADEMESSAGE_H
#include "MessageBase.h"

class TradeMessage : public MessageBase {
public:

    TradeMessage(int id, MessageType type, std::string remark)
            : MessageBase(id, type, std::move(remark)) {

    }




    void set_msg_body( const std::shared_ptr<TradeField>& trade)
    {
        memcpy(&msg_body.trade, trade.get(), sizeof(TradeField));
    }

};


#endif //TRADEBEE_TRADEMESSAGE_H
