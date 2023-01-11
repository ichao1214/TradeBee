//
// Created by haichao.zhang on 2022/3/21.
//

#ifndef TRADEBEE_BARMESSAGE_H
#define TRADEBEE_BARMESSAGE_H

#include "MessageBase.h"

class BarMessage: public MessageBase {

public:

    BarMessage(int id, MessageType type, std::string remark)
            : MessageBase(id, type, std::move(remark)) {

    }

    void set_msg_body(std::shared_ptr<BAR> & bar)
    {
        memcpy(&msg_body.bar, bar.get(), sizeof(msg_body.bar));
    }


};


#endif //TRADEBEE_BARMESSAGE_H
