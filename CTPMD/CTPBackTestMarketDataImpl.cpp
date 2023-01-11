//
// Created by haichao.zhang on 2022/1/30.
//

#include "CTPBackTestMarketDataImpl.h"

#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <iostream>

#include "pubilc.h"
#include "../Message/CTPTickMessage.h"
#include "logger/LogManager.h"

using namespace std;



bool CTPBackTestMarketDataImpl::Init()
{

    LOG_INFO(LOGGER, "ctp md init");
    string ctp_backtest_file = "ctp.ini";

    // 调用boost 文件系统接口，先检查文件是否存在。
    if (!boost::filesystem::exists(ctp_backtest_file))
    {
         LOG_ERROR(LOGGER, "ctp.ini not exists");
        return false;
    }

    boost::property_tree::ptree root_node;
    boost::property_tree::ini_parser::read_ini(ctp_backtest_file, root_node);
    boost::property_tree::ptree account_node;

    auto test_params = root_node.get_child("backtest");
    test_trading_day = test_params.get<string>("test_trading_day");
     LOG_INFO(LOGGER, "backtest trading day {}", test_trading_day);
    printf("backtest trading day %s\n", test_trading_day.c_str());
    trading_day_ = test_trading_day;

    market_data_base_dir = test_params.get<string>("market_data_base_dir");
     LOG_INFO(LOGGER, "market data base dir", market_data_base_dir);
    printf("market data base dir %s\n", market_data_base_dir.c_str());
    current_trading_day_md_file_ = market_data_base_dir + '/' + trading_day_ + "_md";
     LOG_INFO(LOGGER, "current trading day md file {}", current_trading_day_md_file_);
    printf("current trading day md file %s\n", current_trading_day_md_file_.c_str());

    // load and check md bin file
    md_file_ptr_ = std::make_shared<std::ifstream>(current_trading_day_md_file_, ios::binary);

    md_file_ptr_->seekg(0, std::ios::end);
    size_t md_file_length = md_file_ptr_->tellg();

    if ( md_file_length % sizeof(CThostFtdcDepthMarketDataField))
    {
        printf("md file err, length %zu, per md size %lu\n", md_file_length, sizeof(CThostFtdcDepthMarketDataField));
        return false;
    }
    total_tick_counts = md_file_length / sizeof(CThostFtdcDepthMarketDataField);

    printf("md tick count %lu\n", total_tick_counts);
    //tick_buf =   (CThostFtdcDepthMarketDataField*)new char[md_file_length];
    md_file_ptr_->seekg(0, std::ios::beg);

    return true;
}

// symbols always the whole market symbols
void CTPBackTestMarketDataImpl::SubscribeCTPMD(MessageType type, std::vector<std::string> symbols) {
    LOG_INFO(LOGGER, "CTPBackTestMarketDataImpl Subscribe size {}", symbols.size());
    printf("CTPBackTestMarketDataImpl Subscribe size %lu\n", symbols.size());
    printf("SubscribeCTPMD symbols include: ");
    print_container(symbols);
    printf("\n");
    std::set<std::string> notice_symbols_set_(symbols.begin(), symbols.end());
    notice_symbols = notice_symbols_set_;

}

void CTPBackTestMarketDataImpl::StartPushMarketDataThread()  {//start thread to push market data
    quote_push_thread_ = std::make_shared<std::thread>(&CTPBackTestMarketDataImpl::PushMarketData, this);

}

void CTPBackTestMarketDataImpl::JoinPushMarketDataThread() {
    quote_push_thread_->join();
}



std::pair<std::pair<std::string, std::string>,
        std::shared_ptr<CThostFtdcDepthMarketDataField>>
        CTPBackTestMarketDataImpl::ParseCTPQuoteLine(std::string quote_line) {
    std::vector<std::string> quote_fields;
    split(quote_line, ',', quote_fields);
    auto md = std::make_shared<CThostFtdcDepthMarketDataField>();
    char update_time_[32]{};
    sprintf(update_time_,"%s %s", quote_fields[45].c_str(),quote_fields[2].c_str());
    auto key = std::make_pair(string(update_time_), quote_fields[3].c_str());
    strcpy(md->TradingDay, quote_fields[1].c_str());
    strncpy(md->UpdateTime, quote_fields[2].c_str(), 8);
    md->UpdateMillisec = stoi(quote_fields[2].c_str() + 9);
    strcpy(md->InstrumentID, quote_fields[3].c_str());
    strcpy(md->ExchangeID, quote_fields[3].c_str());
    md->LastPrice = stod(quote_fields[6]);
    return std::make_pair(key, md);
}

void CTPBackTestMarketDataImpl::PushMarketData()
{
    char now_time_[13]{};
    get_nowtime(now_time_);

    printf("%s start push md\n", now_time_);
     LOG_INFO(LOGGER, "{} start push md", now_time_);
    auto start = std::chrono::system_clock::now();
    CThostFtdcDepthMarketDataField md{};
    md_file_ptr_->seekg(0, std::ios::beg);
    for(int i = 0; i< total_tick_counts; ++i)
    {

        md_file_ptr_->read((char*)&md, sizeof(CThostFtdcDepthMarketDataField));

        if (notice_symbols.find(md.InstrumentID)!=notice_symbols.end())
        {
            OnRtnDepthMarketData(&md);
        }
    }
    get_nowtime(now_time_);
    printf("%s finish push md\n", now_time_);
     LOG_INFO(LOGGER, "{} finish push md", now_time_);
    auto end = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    printf("total use %ld ms\n", duration.count());
     LOG_INFO(LOGGER, "total use {} ms", duration.count());
}

void CTPBackTestMarketDataImpl::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) {
    auto msg = std::make_shared<CTPTickMessage>(++ctp_tick_num, MessageType::CTPTICKDATA, "ctp_tick");

    msg->set_msg_body(pDepthMarketData);


    PushOrNoticeMsgToAllSubscriber(msg);
}


void CTPBackTestMarketDataImpl::PushCsvMarketData(const vector<std::string>& symbols) {
    for(const auto& s : symbols)
    {
        // load quote data csv
        string file_name{market_data_base_dir + '/' + trading_day_ + "/" + s + ".csv"};
        auto quote_csv_file_ptr = make_shared<ifstream>(file_name);
        LOG_INFO(LOGGER, "load {}", file_name);
        printf("load %s\n", file_name.c_str());
        symbol_quote_file_map_[s] = make_tuple(file_name, quote_csv_file_ptr, false, "", "");

    }
    LOG_INFO(LOGGER, "symbol quote count {}", symbol_quote_file_map_.size());
    printf("symbol quote count %luz\n", symbol_quote_file_map_.size());


    for (const auto & f : symbol_quote_file_map_)
    {
        auto & symbol = f.first;

        auto & quote_file_path = get<0>(f.second);
        auto & quote_file_ptr = get<1>(f.second);

        not_finish_quote.emplace(symbol);

        if (!quote_file_ptr->good())
        {
            printf("symbol %s quote file path %s status err\n", symbol.c_str(), quote_file_path.c_str());
            exit(-1);
        }
        quote_file_ptr->seekg(0, ios_base::end);    // go to the end

        int length =quote_file_ptr->tellg();           // report location (this is the length)
        quote_file_ptr->seekg(0, ios_base::beg);    // go back to the beginning

        printf("symbol %s, quote file %s, file size %d byte\n", symbol.c_str(),quote_file_path.c_str(), length);
    }


    while (!not_finish_quote.empty())
    {
        // 一轮文件读取循环开始，每次读取n，先取n=100
        printf("one read loop start\n");
        for (auto & f : symbol_quote_file_map_)
        {
            //printf("current symbol %s\n", f.first.c_str());
            auto & symbol = f.first;
            auto & quote_file_path = get<0>(f.second);
            auto & quote_file_ptr = get<1>(f.second);
            auto & if_finish = get<2>(f.second);
            auto & first_quote_line = get<3>(f.second);
            auto & last_quote_line = get<4>(f.second);
            if (!quote_file_ptr->good() || if_finish)
            {
                //printf("symbol %s quote file path %s status err or quote finish\n", symbol.c_str(), quote_file_path.c_str());
                continue;
            }
            string quote_line;

            getline(*quote_file_ptr, quote_line);   // head

            if (first_quote_line.empty())
            {
                getline(*quote_file_ptr, first_quote_line);   // first line
                printf("%s quote start, first line %s\n", symbol.c_str(), first_quote_line.c_str());
            }


            std::map<std::string, int> symbol_quote_count_;
            for(const auto& s : not_finish_quote)
            {
                symbol_quote_count_[s] = 0;
            }
            // each file get x lines
            for(int i = 0; i < 100 ; ++i)
            {
                if ( getline(*quote_file_ptr, quote_line).good())
                {
                    //printf("%s quote line: %s\n", symbol.c_str(), quote_line.c_str());
                    // convert line to tick
                    last_quote_line = quote_line;

                }
                else
                {
                    //printf("%s quote file not gold: %s\n", symbol.c_str(), last_quote_line.c_str());
                    // symbol quote finish
                    if_finish = true;
                    printf("%s quote finish, last line %s\n", symbol.c_str(), last_quote_line.c_str());
                    not_finish_quote.erase(symbol);
                    break;
                }

                // parse ctp quote line
                // (update time, symbol) -> md
                auto raw_md = ParseCTPQuoteLine(last_quote_line);
                auto & symbol_ = raw_md.first.second;
                auto & update_time_ = raw_md.first.first;
                auto & md_ = raw_md.second;
                // push the raw to map
                //tmp_all_day_quote_map_.emplace(raw_md);

                //printf("%s %s %f\n", raw_md.first.first.c_str(), raw_md.first.second.c_str(), raw_md.second->LastPrice);
                auto it = symbol_quote_list_.find(symbol_);
                if (it == symbol_quote_list_.end())
                {
                    std::list<std::pair<std::string, std::shared_ptr<CThostFtdcDepthMarketDataField>>> l;
                    l.emplace_back(update_time_, md_);
                    symbol_quote_list_[symbol_] = l;
                }
                else
                {
                    it->second.emplace_back(update_time_, md_);
                }

                // increase tmp symbol left quote
                tmp_symbol_left_quote_map_[raw_md.first.second] ++ ;
                symbol_quote_count_[raw_md.first.second] ++;
            }
        }

        // 一轮文件读取循环结束
        // update time -> (symbol, md)
        printf("one read loop end\n");
        std::multimap<std::string, std::pair<std::string, std::shared_ptr<CThostFtdcDepthMarketDataField>>> md_data;
        for (auto quote_list: symbol_quote_list_)
        {
            // 1 每个合约pop最前面一个数据到一个md_data map中
            auto & symbol_ = quote_list.first;

            if(!quote_list.second.empty())                                    // 该合约的行情list不为空
            {
                auto & data = quote_list.second.front();
                auto & update_time_ = data.first;
                auto & md_ = data.second;
                md_data.emplace(update_time_, std::make_pair(symbol_, md_));
            }
            else if (not_finish_quote.find(symbol_)!=not_finish_quote.end())   // 该合约的行情list为空，且该合约的行情还没回放完成，则继续读取行情文件
            {

                break;
            }
        }


        printf("total md list size %lu\n", md_data.size());
        for (const auto& m: md_data)
        {
            auto & update_time_ = m.first;
            auto & symbol_ = m.second.first;
            auto & md_ = m.second.second;
            printf("%s %s %f\n",update_time_.c_str(), symbol_.c_str(), md_->LastPrice);
        }

        // deal tmp_all_day_quote_map_

        // check the map if contain all unfinished symbol quote
        // 1. 所有合约夜盘回放完成后（没有19-23，0-3点的行情） 夜盘切到日盘
        // 2.

        tmp_symbol_left_quote_map_.clear();
    }

    printf("all quote finish\n");
}