//
// Created by haichao.zhang on 2021/12/30.
//

#ifndef TRADEBEE_CTPMARKETDATAIMPL_H
#define TRADEBEE_CTPMARKETDATAIMPL_H
#include "ThostFtdcMdApi.h"
#include <string>
#include <condition_variable>
#include <fstream>
#include "../StrategyBase/StrategyBase.h"
#include "CTPMarketDataBase.h"



class CTPMarketDataImpl : public CTPMarketDataBase, public CThostFtdcMdSpi
{
private:
    CThostFtdcMdApi * m_pMdApi;
    int is_real; //
    int if_record;
    bool connected{};
    std::mutex connected_mxt;
    std::condition_variable connected_cv;

    bool login_flag = false;
    std::mutex login_mxt;
    std::condition_variable login_cv;

    std::string trade_front_;
    std::string quote_front_;
    std::string broker_id_;
    std::string user_id_;
    std::string password_;
    std::string app_id_;
    std::string auth_code_;

    int m_request_id = 0;

    std::string trading_day;

    int ctp_tick_num = 0;



    std::shared_ptr<std::ofstream> md_record_file_stream_ptr_;

    std::string md_record_name_;

    std::string md_record_dir_;

public:
    void OnFrontConnected() override;

    void OnFrontDisconnected(int nReason) override;

    void OnHeartBeatWarning(int nTimeLapse) override;


    void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) override;

    bool Login(std::string username, std::string password, char** paras = nullptr) override;

    bool Init() override;

    int SubscribeMarketData();

    void SubscribeCTPMD(MessageType type, std::vector<std::string> symbols) override;

    ~CTPMarketDataImpl()
    {
        if (md_record_file_stream_ptr_ &&  md_record_file_stream_ptr_->is_open())
        {
            md_record_file_stream_ptr_->close();
            printf("md record file close\n");
        }
    }
};


#endif //TRADEBEE_CTPMARKETDATAIMPL_H
