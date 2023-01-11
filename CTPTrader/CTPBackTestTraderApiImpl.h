//
// Created by zhanghaichao on 2021/12/22.
//

#ifndef TRADEBEE_CTPBACKTESTTRADERAPIIMPL_H
#define TRADEBEE_CTPBACKTESTTRADERAPIIMPL_H

#include "../StrategyBase/StrategyBase.h"

#include "CTPTraderBase.h"
#include "../BackTestEngine/SimpleBackTestEngine.h"
#include "../BackTestEngine/BackTestTraderSpi.h"

#include <map>
#include <cstring>


class GlobalData;

class CTPBackTestTraderApiImpl :public CTPTraderBase, public BackTestTraderSpi{
private:


    std::shared_ptr<BackTestEngineBase> back_test_engine_api;



public:
    CTPBackTestTraderApiImpl(const std::shared_ptr<BackTestEngineBase>& backTestEngineBase)
    {
        printf("backtest engine add %x\n", backTestEngineBase.get());
        back_test_engine_api = backTestEngineBase;
    }

    ~CTPBackTestTraderApiImpl() override
    {
        printf("~CTPBackTestTraderApiImpl dtor\n");
    }

    int Release() override
    {
        return 0;
    }


    bool Init() override;

    bool Login(std::string username, std::string password, char** paras) override;

    std::shared_ptr<OrderField> InsertLimitPriceOrder(std::string symbol, OrderDirection direction, OrderOffsetFlag offsetFlag,
                              int volume, double price, std::shared_ptr<StrategyBase> strategy) override ;

    bool CancelCtpOrder(std::string exchange_id, std::string order_sys_id, int order_id, std::string symbol) override;

    std::shared_ptr<OrderField> InsertStopOrder(std::string symbol, OrderDirection direction, OrderOffsetFlag offsetFlag,
                        int volume, double price,std::shared_ptr<StrategyBase> strategy) override ;

    void OnBackTestRtnOrder(std::shared_ptr<OrderField>& order) override;

    void OnBackTestRtnTrade(std::shared_ptr<OrderField>& order, std::shared_ptr<TradeField>& trade) override;


    static bool InitFromBinFile();
};


#endif //TRADEBEE_CTPBACKTESTTRADERAPIIMPL_H
