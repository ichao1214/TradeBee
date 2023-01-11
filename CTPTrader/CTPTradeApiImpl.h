//
// Created by zhanghaichao on 2021/12/17.
//

#ifndef TRADEBEE_CTPTRADEAPIIMPL_H
#define TRADEBEE_CTPTRADEAPIIMPL_H

#include <memory>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <set>
#include <map>
#include <vector>

#include "ThostFtdcTraderApi.h"
#include "../StrategyBase/StrategyBase.h"
#include "CTPTraderBase.h"


class CTPTradeApiImpl: public CTPTraderBase , public CThostFtdcTraderSpi
{
private:

    CThostFtdcTraderApi * m_pTradeApi{};


    bool connected{};
    std::mutex connected_mxt;
    std::condition_variable connected_cv;

    bool login_flag = false;
    std::mutex login_mxt;
    std::condition_variable login_cv;



    int is_real; //
    int if_record;
    int if_qry_commi_ = 0;
    int if_qry_margin_ = 0;
    int if_qry_quote_ = 0;

    std::string trade_record_dir_;

    std::string trade_front_;
    std::string quote_front_;
    std::string broker_id_;
    std::string user_id_;
    std::string password_;
    std::string app_id_;
    std::string auth_code_;

    std::mutex qry_mtx_;
    std::condition_variable qry_cv_;
    bool qry_flag_ = false;

    // login info

    int front_id_;
    int session_id_;
    std::string max_order_ref_;




    CThostFtdcTradingAccountField account_info_;
    std::map<std::string, std::vector<CThostFtdcOrderField>> ctp_order_map_;
    std::map<std::string, std::vector<CThostFtdcTradeField>> ctp_trade_map_;
    std::map<std::string,
    std::pair<std::vector<CThostFtdcInvestorPositionField>,
            std::vector<CThostFtdcInvestorPositionField>>> position_map_;

    std::unordered_map<int, std::string> order_id_ctp_order_ref_map_;

public:

    int Release() override
    {
        return 0;
    }


    bool Init() override;

    bool Login(const std::string username, const std::string password, char** paras = nullptr) override;


    std::shared_ptr<OrderField> InsertLimitPriceOrder(std::string symbol, OrderDirection direction, OrderOffsetFlag offsetFlag,
                                                      int volume, double price, std::shared_ptr<StrategyBase> strategy) override ;

    bool CancelCtpOrder(std::string exchange_id, std::string order_sys_id, int order_id, std::string symbol) override;

    std::shared_ptr<OrderField>  InsertStopOrder(const std::string symbol, OrderDirection direction, OrderOffsetFlag offsetFlag,
                        int volume, double price,  std::shared_ptr<StrategyBase> strategy) override;

    void OnFrontConnected(void) override;
    void OnFrontDisconnected(int nReason) override;
    void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField,
                                   CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID,
                                   bool bIsLast) override;
    void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                                CThostFtdcRspInfoField *pRspInfo,
                                int nRequestID,
                                bool bIsLast) override;

    void OnRspOrderInsert(CThostFtdcInputOrderField *pOrder,
                                          CThostFtdcRspInfoField *pRspInfo,
                                          int nRequestID,
                                          bool bIsLast) override;

    void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder,
                                     CThostFtdcRspInfoField *pRspInfo) override;

    void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    void OnRtnOrder(CThostFtdcOrderField *pOrder) override;

    void OnRtnTrade(CThostFtdcTradeField *pTrade) override;

    void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo) override;

    void OnRspQryClassifiedInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

};

#endif //TRADEBEE_CTPTRADEAPIIMPL_H
