//
// Created by haichao.zhang on 2022/1/30.
//

#ifndef TRADEBEE_CTPBACKTESTMARKETDATAIMPL_H
#define TRADEBEE_CTPBACKTESTMARKETDATAIMPL_H

#include "ThostFtdcMdApi.h"
#include "CTPMarketDataBase.h"
#include <set>
#include <map>
#include <thread>
#include "../StrategyBase/StrategyBase.h"

class CTPBackTestMarketDataImpl : public CTPMarketDataBase{

public:

    CTPBackTestMarketDataImpl()
    {
        printf("CTPBackTestMarketDataImpl ctor\n");
    }

    bool Init() override;

    void SubscribeCTPMD(MessageType type, std::vector<std::string> symbols) override;


    ~CTPBackTestMarketDataImpl()
    {
        printf("~CTPBackTestMarketDataImpl dtor\n");
        md_file_ptr_->close();
//        if (tick_buf)
//            free(tick_buf);
    }
private:
    std::string test_trading_day;
    std::string trading_day_;

    std::string market_data_base_dir;

    std::string current_trading_day_md_file_;


    std::set<std::string> notice_symbols;

    std::set<std::string> not_finish_quote;

    std::shared_ptr<std::ifstream> md_file_ptr_;

    std::map<std::string, int> tmp_symbol_left_quote_map_;

    CThostFtdcDepthMarketDataField *tick_buf;

    unsigned long total_tick_counts;

    int ctp_tick_num = 0;

    std::shared_ptr<std::thread> quote_push_thread_;

    // symbol name : quote file path , file ptr, if finish , first line , last line
    std::map<std::string, std::tuple<std::string, std::shared_ptr<std::ifstream>, bool,
                std::string , std::string >> symbol_quote_file_map_;

    // symbol -> (update time, md)
    std::map<std::string,
           std::list<std::pair<std::string,std::shared_ptr<CThostFtdcDepthMarketDataField>>>>
            symbol_quote_list_;

    typedef
    std::pair<std::pair<std::string, std::string>,
    std::shared_ptr<CThostFtdcDepthMarketDataField>> RawCTPMarketData;

    // (update time, symbol) -> quote line
    std::multimap<std::pair<std::string, std::string>,
            std::shared_ptr<CThostFtdcDepthMarketDataField>> tmp_all_day_quote_map_;

public:
    // return (update time, symbol) -> md
    static RawCTPMarketData
    ParseCTPQuoteLine(std::string quote_line);



    void PushMarketData();

    void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);


    void StartPushMarketDataThread() override ;

    void JoinPushMarketDataThread() override;


    // 废弃
    void PushCsvMarketData(const std::vector<std::string>& symbols);
};


#endif //TRADEBEE_CTPBACKTESTMARKETDATAIMPL_H
