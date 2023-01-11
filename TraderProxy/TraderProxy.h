//
// Created by zhanghaichao on 2021/12/19.
//

#ifndef TRADEBEE_TRADERPROXY_H
#define TRADEBEE_TRADERPROXY_H


#include <memory>
#include <utility>

#include "../TraderBase/TraderBase.h"
#include "../CTPTrader/CTPTradeApiImpl.h"
#include "../CTPTrader/CTPBackTestTraderApiImpl.h"

class TraderProxy: public TraderBase{
public:
    explicit TraderProxy(std::string type, std::shared_ptr<BackTestEngineBase> backTestEngineBase)
    :m_type(std::move(type)), back_test_engine(std::move(backTestEngineBase))
    {
        real_trader = CreateRealTraderInstance();
    };

    explicit TraderProxy(std::string type)
            :m_type(std::move(type))
    {
        real_trader = CreateRealTraderInstance();
    };

    std::shared_ptr<TraderBase> CreateRealTraderInstance()
    {
        if (m_type == "ctp")
        {
            return std::make_shared<CTPTradeApiImpl>();
        }
        else if(m_type == "backtest")
        {
            return std::make_shared<CTPBackTestTraderApiImpl>(back_test_engine);
        }
        else
        {
            printf("not support type: %s", m_type.c_str());
            throw "create real trader error";
        }
    }

    bool Init() override;

    bool Login(const std::string username, const std::string password, char** paras = nullptr) override;

    int Release() override
    {
        return 0;
    }

    std::shared_ptr<OrderField>  Buy(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy) override;
    std::shared_ptr<OrderField>  Sell(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy) override;
    std::shared_ptr<OrderField>  Short(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy) override;
    std::shared_ptr<OrderField>  Cover(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy) override;
    bool Cancel(std::shared_ptr<OrderField>  order, std::shared_ptr<StrategyBase> strategy) override;

    std::vector<std::string> get_all_symbols() override
    {
        return real_trader->get_all_symbols();
    }


    SymbolField * get_symbol_info(std::string symbol) override
    {
        return real_trader->get_symbol_info(symbol);
    }

    std::shared_ptr<CommiField> getSymbolCommi(std::string& Symbol)
    {
        return real_trader->getSymbolCommi(Symbol);
    }

    virtual std::shared_ptr<MarginField> getSymbolMargin(std::string& Symbol)
    {
        return real_trader->getSymbolMargin(Symbol);
    }

    virtual std::pair<std::string, std::string> getProductMainSymbol(std::string & Product)
    {
        return real_trader->getProductMainSymbol(Product);
    }

private:
    std::string m_type;
    std::shared_ptr<TraderBase> real_trader;
    std::shared_ptr<BackTestEngineBase> back_test_engine;
};


#endif //TRADEBEE_TRADERPROXY_H
