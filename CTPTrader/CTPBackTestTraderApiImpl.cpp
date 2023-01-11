//
// Created by zhanghaichao on 2021/12/22.
//

#include "CTPBackTestTraderApiImpl.h"
#include "logger/LogManager.h"
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include "pubilc.h"
#include "../BackTestEngine/SimpleBackTestEngine.h"




std::string TraderBase::trading_day_;


bool CTPBackTestTraderApiImpl::InitFromBinFile() {

     LOG_INFO(LOGGER, "ctp backtest trader init");
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
    trading_day_ = test_params.get<string>("test_trading_day");
     LOG_INFO(LOGGER, "backtest trading day {}", trading_day_);
    printf("backtest trading day %s\n", trading_day_.c_str());

    GlobalData::g_trading_day_ = trading_day_;
    GlobalData::g_trading_day_init_flag_ = true;

    basic_data_base_dir = test_params.get<string>("basic_data_base_dir");
     LOG_INFO(LOGGER, "basic data base dir", basic_data_base_dir);
    printf("basic data base dir %s\n", basic_data_base_dir.c_str());

    std::ifstream inst_csv_file{basic_data_base_dir+ '/' + trading_day_+ "/inst_info.csv"};
    std::ifstream commi_csv_file{basic_data_base_dir + '/' + trading_day_+ "/commi.csv"};
    std::ifstream margin_csv_file{basic_data_base_dir + '/' + trading_day_+ "/margin.csv"};


    InitCTPSymbolsFromBinFile();
    InitMarginFromBinFile();
    InitCommiFromBinFile();
    InitMarketFromBinFile();
    InitMainSymbol();

    return true;
}

bool CTPBackTestTraderApiImpl::Init()
{
     LOG_INFO(LOGGER, "ctp backtest trader init");
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

//    auto test_params = root_node.get_child("backtest");
//    test_trading_day = test_params.get<string>("test_trading_day");
//     LOG_INFO(LOGGER, "backtest trading day {}", test_trading_day);
//    printf("backtest trading day %s\n", test_trading_day.c_str());
//    trading_day_ = test_trading_day;
//    GlobalData::g_trading_day_ = trading_day_;
//    GlobalData::g_trading_day_init_flag_ = true;
//
//    basic_data_base_dir = test_params.get<string>("basic_data_base_dir");
//     LOG_INFO(LOGGER, "basic data base dir", basic_data_base_dir);
//    printf("basic data base dir %s\n", basic_data_base_dir.c_str());

//    std::ifstream inst_csv_file{basic_data_base_dir+ '/' + trading_day_+ "/inst_info.csv"};
//    std::ifstream commi_csv_file{basic_data_base_dir + '/' + trading_day_+ "/commi.csv"};
//    std::ifstream margin_csv_file{basic_data_base_dir + '/' + trading_day_+ "/margin.csv"};

//    InitFromBinFile();

    //GlobalData::InitSymbolIndex();
//    if (GlobalData::g_symbol_init_flag_.load() == false) {
//        GlobalData::g_symbol_init_flag_.store(true);
//
//        GlobalData::InitSymbolIndex();
//    }


    for_each(inst_map_.begin(), inst_map_.end(), [](std::pair<std::string, CThostFtdcInstrumentField> i)
    {
        printf("symbol: %s %s %f %d\n", i.first.c_str(), i.second.ExchangeID, i.second.PriceTick, i.second.VolumeMultiple);
    });


    if (back_test_engine_api== nullptr)
    {
        printf("backtest engine null\n");
        return false;
    }
    back_test_engine_api->RegisterSpi(GetSharedPtr());
    back_test_engine_api->Init();
    return true;
}




bool CTPBackTestTraderApiImpl::Login(const std::string username, const std::string password, char **paras) {
     LOG_INFO(LOGGER, "ctp backtest login success");
    return true;
}


std::shared_ptr<OrderField>  CTPBackTestTraderApiImpl::InsertLimitPriceOrder(std::string symbol, OrderDirection direction,
                                                        OrderOffsetFlag offsetFlag, int volume, double price, std::shared_ptr<StrategyBase> strategy) {
    auto order = std::make_shared<OrderField>();
    order->OrderID = GetOrderID();
    strcpy(order->Symbol, symbol.c_str());
    order->Direction = direction;
    order->OffsetFlag = offsetFlag;
    order->Volume = volume;
    order->Price = price;
    order->strategy = strategy;
    order->Status = OrderStatus::UNKNOWN;
    back_test_engine_api->ReqInsertOrder(order);
    return order;
}

std::shared_ptr<OrderField>  CTPBackTestTraderApiImpl::InsertStopOrder(std::string symbol, OrderDirection direction,
                                                  OrderOffsetFlag offsetFlag, int volume, double price, std::shared_ptr<StrategyBase> strategy) {
    return nullptr;
}


void CTPBackTestTraderApiImpl::OnBackTestRtnOrder(shared_ptr<OrderField> &order) {


     LOG_INFO(LOGGER, "CTP OnBackTestRtnOrder: "
                      "strategy name {}, "
                      "symbol {}, order id {}, "
                 "ctp backtest order sys id {}, "
                 "direction {}, "
                 "offset {}, "
                 "price {}, total volume original {}, volume traded {}, status {}",
                          order->strategy.lock()->strategy_name.c_str(),
                          order->Symbol, order->OrderID,
                          order->OrderRef,
                 order->Direction == OrderDirection::BUY ? "Buy" : "Sell",
                 (order->OffsetFlag == OrderOffsetFlag::OPEN ) ? "Open" : order->OffsetFlag == OrderOffsetFlag::CLOSE ? "Close" : "CloseYesterday",
                          order->Price, order->Volume, order->VolumeTraded, order->Status);

//    printf("CTP OnBackTestRtnOrder: strategy name %s, symbol %s, order id %d, "
//           "ctp backtest order sys id %s, "
//           "direction %s, "
//           "offset %s, "
//           "price %.2lf, total volume original %d, volume traded %d, status %d\n",
//           order->strategy->GetStrategyName().c_str(), order->Symbol, order->OrderID,
//           order->OrderRef,
//           order->Direction == OrderDirection::BUY ? "Buy" : "Sell",
//           (order->OffsetFlag == OrderOffsetFlag::OPEN ) ? "Open" : order->OffsetFlag == OrderOffsetFlag::CLOSE ? "Close" : "CloseYesterday",
//           order->Price, order->Volume, order->VolumeTraded, order->Status);

    traderCallBack->OnRtnOrder(order);
}


void
CTPBackTestTraderApiImpl::OnBackTestRtnTrade(shared_ptr<OrderField> &order, shared_ptr<TradeField> &trade) {

     LOG_INFO(LOGGER, "CTP OnBackTestRtnTrade: strategy name {}, symbol {}, trade id {},order id {}, "
                 "ctp order sys id {}, "
                 "direction {}, "
                 "offset {}, "
                 "price {}, trade volume {}, total volume original {}, total volume traded {}, status {}",
                          order->strategy.lock()->strategy_name, order->Symbol, trade->TradeID, order->OrderID,
                          order->OrderRef,
                 order->Direction == OrderDirection::BUY ? "Buy" : "Sell",
                 (order->OffsetFlag == OrderOffsetFlag::OPEN ) ? "Open" : order->OffsetFlag == OrderOffsetFlag::CLOSE ? "Close" : "CloseYesterday",
                          order->Price, trade->TradeVolume, order->Volume, order->VolumeTraded, order->Status);

//    printf("CTP OnBackTestRtnTrade: strategy name %s, symbol %s, order id %d, "
//           "ctp order sys id %s, "
//           "direction %s, "
//           "offset %s, "
//           "price %.2lf, total volume original %d, volume traded %d, status %d\n",
//           order->strategy->GetStrategyName().c_str(), order->Symbol, order->OrderID,
//           order->OrderRef,
//           order->Direction == OrderDirection::BUY ? "Buy" : "Sell",
//           (order->OffsetFlag == OrderOffsetFlag::OPEN ) ? "Open" : order->OffsetFlag == OrderOffsetFlag::CLOSE ? "Close" : "CloseYesterday",
//           order->Price, order->Volume, order->VolumeTraded, order->Status);
    traderCallBack->OnRtnTrade(order, trade);


}

bool CTPBackTestTraderApiImpl::CancelCtpOrder(std::string exchange_id, std::string order_sys_id, int order_id, std::string symbol) {

    return back_test_engine_api->ReqCancelOrder(exchange_id, order_sys_id, order_id, symbol);
}




