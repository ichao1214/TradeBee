//
// Created by haichao.zhang on 2022/4/2.
//

#ifndef TRADEBEE_BACKTESTTRADERSPI_H
#define TRADEBEE_BACKTESTTRADERSPI_H

#include "../StrategyBase/StrategyBase.h"

class BackTestTraderSpi : public std::enable_shared_from_this<BackTestTraderSpi> {
public:
    std::shared_ptr<BackTestTraderSpi> GetSharedPtr(){
        return shared_from_this();
    }

    virtual void OnBackTestRtnOrder(std::shared_ptr<OrderField>& order) = 0;

    virtual void OnBackTestRtnTrade(std::shared_ptr<OrderField>& order, std::shared_ptr<TradeField>& trade) = 0;
};


#endif //TRADEBEE_BACKTESTTRADERSPI_H
