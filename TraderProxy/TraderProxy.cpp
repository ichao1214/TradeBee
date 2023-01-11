//
// Created by zhanghaichao on 2021/12/19.
//

#include "TraderProxy.h"

bool TraderProxy::Init()
{
    return real_trader->Init();
}

bool TraderProxy::Login(const std::string username, const std::string password, char** paras)
{
    return real_trader->Login(username, password, paras);
}

std::shared_ptr<OrderField> TraderProxy::Buy(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy) {
    return real_trader->Buy(symbol, price, volume, strategy);
}

std::shared_ptr<OrderField> TraderProxy::Sell(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy) {
    return real_trader->Sell(symbol,price, volume, strategy);
}

std::shared_ptr<OrderField> TraderProxy::Short(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy) {
    return real_trader->Short(symbol,price, volume, strategy);
}

std::shared_ptr<OrderField> TraderProxy::Cover(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy) {
    return real_trader->Cover(symbol, price, volume, strategy);
}

bool TraderProxy::Cancel(std::shared_ptr<OrderField> orderField,std::shared_ptr<StrategyBase> strategy) {
    return real_trader->Cancel(orderField, strategy);
}
