//
// Created by zhanghaichao on 2021/12/17.
//
#include <string>
#include <memory>

#ifndef TRADEBEE_BASETRADER_H
#define TRADEBEE_BASETRADER_H

#include "TraderCallBackBase.h"

#include <utility>
#include <vector>
#include <map>
#include <unordered_map>



class TraderCallBackBase;

class TraderBase {
public:
    TraderBase()
    {
        order_id = 0;
    }

    virtual ~TraderBase()
    = default;

    virtual bool Init()
    {
        return true;
    }

    virtual bool Login(const std::string username, const std::string password, char** paras = nullptr)
    {
        return true;
    }

    virtual void RegisterTraderSpi(std::shared_ptr<TraderCallBackBase> traderSpi)
    {
        traderCallBack = std::move(traderSpi);
    }

    std::shared_ptr<TraderCallBackBase> GetTraderSpi()
    {
        return traderCallBack;
    }

    void AppendNewOrder(int new_order_id, std::shared_ptr<OrderField> & new_order)
    {
        std::lock_guard<std::mutex> lock(orders_map_lock_);
        orders_map_.emplace(new_order_id, new_order);
    }

    std::shared_ptr<OrderField> GetOrderByID(int order_id)
    {
        std::lock_guard<std::mutex> lock(orders_map_lock_);
        auto it = orders_map_.find(order_id);
        if(it != orders_map_.end())
        {
            return it->second;
        }
        return nullptr;
    }

    int GetOrderID()
    {
        return ++order_id;
    }


    virtual int Release() = 0;

    virtual std::shared_ptr<OrderField> Buy(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy) = 0 ;
    virtual std::shared_ptr<OrderField> Sell(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy) = 0;
    virtual std::shared_ptr<OrderField> Short(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy) = 0;
    virtual std::shared_ptr<OrderField> Cover(std::string symbol, double price,int volume, std::shared_ptr<StrategyBase> strategy) = 0;
    virtual bool Cancel(std::shared_ptr<OrderField> orderField, std::shared_ptr<StrategyBase> strategy) = 0;

    virtual std::vector<std::string> get_all_symbols() = 0;

    virtual SymbolField * get_symbol_info(std::string symbol) = 0;


    virtual std::shared_ptr<CommiField> getSymbolCommi(std::string& Symbol) = 0;

    virtual std::shared_ptr<MarginField> getSymbolMargin(std::string& Symbol) = 0;

    virtual std::pair<std::string, std::string> getProductMainSymbol(std::string & Product) = 0 ;

private:



protected:
    static std::string trading_day_;
    std::shared_ptr<TraderCallBackBase> traderCallBack;

    std::mutex orders_map_lock_;
    std::unordered_map<int, std::shared_ptr<OrderField>> orders_map_;

    int order_id;


};





#endif //TRADEBEE_BASETRADER_H
