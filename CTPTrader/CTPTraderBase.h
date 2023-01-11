//
// Created by haichao.zhang on 2022/3/19.
//

#ifndef TRADEBEE_CTPTRADERBASE_H
#define TRADEBEE_CTPTRADERBASE_H

#include <map>
#include <set>

#include "ThostFtdcTraderApi.h"

#include "../StrategyBase/StrategyBase.h"
#include "../TraderBase/TraderBase.h"
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include "CTPTraderCallBack.h"




class GlobalData;

class CTPTraderBase: public TraderBase  {

public:
    CTPTraderBase()
    {
        //LoadTradingPeriod();
        traderCallBack = std::make_shared<CTPTraderCallBack>();
    }

    //int LoadTradingPeriod();

    ~CTPTraderBase()
    {

    }

    std::shared_ptr<OrderField> Buy(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy) override;
    std::shared_ptr<OrderField> Sell(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy) override;
    std::shared_ptr<OrderField> Short(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy) override;
    std::shared_ptr<OrderField> Cover(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy) override;
    bool Cancel(std::shared_ptr<OrderField>  order, std::shared_ptr<StrategyBase>strategy) override;

    virtual std::shared_ptr<OrderField> InsertLimitPriceOrder(std::string symbol, OrderDirection direction, OrderOffsetFlag offsetFlag,
                              int volume, double price, std::shared_ptr<StrategyBase> strategy) = 0;

    virtual bool CancelCtpOrder(std::string exchange_id, std::string order_sys_id, int order_id, std::string symbol) = 0;


    virtual std::shared_ptr<OrderField> InsertStopOrder(std::string symbol, OrderDirection direction, OrderOffsetFlag offsetFlag,
                        int volume, double price, std::shared_ptr<StrategyBase> strategy) = 0;



    static void InitCTPSymbolsFromBinFile();
    static void InitCommiFromBinFile();
    static void InitMarginFromBinFile();
    static void InitMarketFromBinFile();
    static void InitMainSymbol() ;

    std::vector<std::string> get_all_symbols() override
    {
        std::vector<std::string> symbols;
        for (const auto & i : inst_map_)
        {
            symbols.push_back(i.first);
        }
        return symbols;
    }

    SymbolField * get_symbol_info(std::string symbol) override
    {
        auto it = inst_map_.find(symbol);
        if (it == inst_map_.end())
        {
            return nullptr;
        }
        auto symbol_field = new SymbolField;
        strcpy(symbol_field->Symbol,it->second.InstrumentID);
        symbol_field->ExchangeId = ExchangeIDConvert(it->second.ExchangeID);
        symbol_field->PriceTick = it->second.PriceTick;
        symbol_field->Multiple = it->second.VolumeMultiple;
        strcpy(symbol_field->ProductID,it->second.ProductID);
        return symbol_field;
    }

    inline int GetCtpOrderID()
    {
        return ++m_request_id;
    }

    void AppendOrderIDMap(int ctp_order_id, int order_id)
    {
        std::lock_guard<std::mutex> l(ctp_order_id_order_id_map_lock);
        ctp_order_id_order_id_map_[ctp_order_id] = order_id;
    }

    int GetOrderIDByCtpRequestID(int ctp_request_id)
    {
        std::lock_guard<std::mutex> l(ctp_order_id_order_id_map_lock);
        auto it = ctp_order_id_order_id_map_.find(ctp_request_id);
        if (it != ctp_order_id_order_id_map_.end())
        {
            return it->second;
        }
        return -1;
    }

    void AppendOrderSysIDMap(const std::string& ctp_order_sys_id, int order_id)
    {
        std::lock_guard<std::mutex> l(ctp_order_sys_id_order_id_map_lock);
        ctp_order_sys_id_order_id_map_[ctp_order_sys_id] = order_id;
    }

    int GetOrderIDByCtpOrderSysID(const std::string& ctp_order_sys_id)
    {
        std::lock_guard<std::mutex> l(ctp_order_sys_id_order_id_map_lock);
        auto it = ctp_order_sys_id_order_id_map_.find(ctp_order_sys_id);
        if (it != ctp_order_sys_id_order_id_map_.end())
        {
            return it->second;
        }
        return -1;
    }


    OrderStatus CTPOrderStatusConvert(TThostFtdcOrderStatusType ctp_order_status)
    {
        OrderStatus status = OrderStatus::UNKNOWN;
        switch (ctp_order_status)
        {
            case THOST_FTDC_OST_AllTraded:
                status = OrderStatus::ALL_TRADED;
                break;
            case THOST_FTDC_OST_PartTradedQueueing:
                status = OrderStatus::PART_TRADED;
                break;
            case THOST_FTDC_OST_PartTradedNotQueueing:
                status = OrderStatus::CANCELED;
                break;
            case THOST_FTDC_OST_NoTradeQueueing:
                status = OrderStatus::ACCEPTED;
                break;
            case THOST_FTDC_OST_NoTradeNotQueueing:
                status = OrderStatus::CANCELED;
                break;
            case THOST_FTDC_OST_Canceled:
                status = OrderStatus::CANCELED;
                break;
            case THOST_FTDC_OST_Unknown:
                status = OrderStatus::UNKNOWN;
                break;
            case THOST_FTDC_OST_NotTouched:
                status = OrderStatus::UNKNOWN;
                break;
            case THOST_FTDC_OST_Touched:
                status = OrderStatus::UNKNOWN;
                break;
        }
        return status;
    }
public:

    std::shared_ptr<CommiField> getSymbolCommi(std::string& Symbol)  override;

    std::shared_ptr<MarginField> getSymbolMargin(std::string& Symbol)  override;

    std::pair<std::string, std::string> getProductMainSymbol(std::string & Product) override ;

protected:

    static std::map<std::string, CThostFtdcInstrumentField> inst_map_;
    static std::map<std::string, CThostFtdcDepthMarketDataField> market_map_;

    static std::map<std::string, CThostFtdcInstrumentCommissionRateField>  commi_map_;
    static std::map<std::string, CThostFtdcInstrumentMarginRateField> margin_map_;

    static std::map<std::string, std::multimap<int, std::string>> product_pre_open_intetest_map_;
    static std::map<std::string, std::pair<std::string, std::string>> product_main_symbol_map_;


    static std::string basic_data_base_dir;

    std::set<std::string> concern_inst_;

    int m_request_id = 0;

    std::mutex ctp_order_id_order_id_map_lock;
    std::unordered_map<int, int> ctp_order_id_order_id_map_;


    std::mutex ctp_order_sys_id_order_id_map_lock;
    std::unordered_map<std::string, int> ctp_order_sys_id_order_id_map_;

private:

};


#endif //TRADEBEE_CTPTRADERBASE_H
