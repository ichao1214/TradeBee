//
// Created by haichao.zhang on 2021/12/31.
//

#ifndef TRADEBEE_MESSAGEBASE_H
#define TRADEBEE_MESSAGEBASE_H
#include <string>
#include <memory>
#include <list>
#include "../DataStruct/TradeBeeDataStruct.h"



class MessageBase {

protected:
    MSG_BODY msg_body;

private:
    MessageType type;
    std::string remark;
    int global_msg_id;
public:
    MessageBase ()
    {

    }

    MessageBase(int id, MessageType type, std::string remark):global_msg_id(id), type(type), remark(remark)
    {

    }

    ~MessageBase()
    {
        //printf("~MessageBase: id %d, type %d, remark %s\n", global_msg_id, type, remark.c_str());
    }

    MessageBase& operator=(std::shared_ptr<MessageBase> msg)
    {
        type = msg->get_msg_type();
        remark = msg->get_msg_remark();
        global_msg_id = msg->get_msg_id();
        msg_body = msg->get_msg_body();
        return *this;
    }

    MessageBase& operator=(MessageBase msg)
    {
        type = msg.get_msg_type();
        remark = msg.get_msg_remark();
        global_msg_id = msg.get_msg_id();
        msg_body = msg.get_msg_body();
        msg_body.tick = msg.get_msg_body().tick;
        return *this;
    }

    MSG_BODY get_msg_body()
    {
        return msg_body;
    }

    int get_msg_id()
    {
        return global_msg_id;
    }

    MessageType get_msg_type()
    {
        return type;
    }

    std::string get_msg_remark()
    {
        return remark;
    }
};


#endif //TRADEBEE_MESSAGEBASE_H
