//
// Created by haichao.zhang on 2021/12/30.
//

#include "CTPMarketDataImpl.h"
#include "logger/LogManager.h"
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "../Message/CTPTickMessage.h"
#include "../EventBase/EventSubscriberBase.h"

using namespace std;



bool CTPMarketDataImpl::Init() {
    string ctp_ini_file = "ctp.ini";
    // 调用boost 文件系统接口，先检查文件是否存在。
    if (!boost::filesystem::exists(ctp_ini_file))
    {
         LOG_ERROR(LOGGER, "ctp.ini not exists");
        return false;
    }

    boost::property_tree::ptree root_node;
    boost::property_tree::ini_parser::read_ini(ctp_ini_file, root_node);
    boost::property_tree::ptree account_node;

    auto run_mode = root_node.get_child("run_mode");
    is_real = run_mode.get<int>("is_real");
    if(is_real == 1)
    {
         LOG_INFO(LOGGER, "real mode");
        account_node = root_node.get_child("ctp");

        if_record =  run_mode.get<int>("record_md");
        if(if_record == 1)
        {
            md_record_dir_ =  run_mode.get<string>("record_dir");
             LOG_INFO(LOGGER, "md record on, md dir {}", md_record_dir_);
        }
    }
    else
    {
         LOG_INFO(LOGGER, "test mode");
        account_node = root_node.get_child("simnow");
    }


    trade_front_ = account_node.get<string>("trade_front");
    quote_front_ = account_node.get<string>("quote_front");
    broker_id_ =account_node.get<string>("broker_id");
    user_id_ = account_node.get<string>("user_id");
    password_ = account_node.get<string>("password");
    app_id_ = account_node.get<string>("app_id");
    auth_code_ = account_node.get<string>("auth_code");
     LOG_DEBUG(LOGGER, "trade_front {}", trade_front_);
     LOG_DEBUG(LOGGER, "quote_front {}", quote_front_);
     LOG_DEBUG(LOGGER, "broker_id {}", broker_id_);
     LOG_DEBUG(LOGGER, "user_id {}", user_id_);
     LOG_DEBUG(LOGGER, "password {}", password_);
     LOG_DEBUG(LOGGER, "app_id {}", app_id_);
     LOG_DEBUG(LOGGER, "auth_code {}", auth_code_);

    auto* version = CThostFtdcMdApi::GetApiVersion();

    m_pMdApi = CThostFtdcMdApi::CreateFtdcMdApi();

     LOG_INFO(LOGGER, CThostFtdcMdApi::GetApiVersion());

    m_pMdApi->RegisterSpi(this);
    m_pMdApi->RegisterFront(const_cast<char *>(quote_front_.c_str()));
    m_pMdApi->Init();
    return true;

}

void CTPMarketDataImpl::OnFrontConnected()
{
     LOG_INFO(LOGGER, "MD: OnFrontConnected!");

    {
        std::lock_guard<std::mutex> lg(connected_mxt);
        connected = true;
    }
    connected_cv.notify_one();
}

void CTPMarketDataImpl::OnFrontDisconnected(int nReason)
{
     LOG_WARN(LOGGER, "OnMdFrontDisconnected");

    {
        std::lock_guard<std::mutex> lg(connected_mxt);
        connected = false;
    }
    connected_cv.notify_one();
}

void CTPMarketDataImpl::OnHeartBeatWarning(int nTimeLapse)
{
     LOG_INFO(LOGGER, "OnMdHeartBeatWarning");
}

void CTPMarketDataImpl::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (pRspInfo->ErrorID == 0)
    {
        {
            std::lock_guard<std::mutex> lg(login_mxt);
            login_flag = true;
        }
        login_cv.notify_one();
        trading_day = string(pRspUserLogin->TradingDay);
         LOG_INFO(LOGGER, "user login md success, TradingDay {}", trading_day);
        if (!GlobalData::g_trading_day_init_flag_)
        {
            GlobalData::g_trading_day_ = trading_day;
            GlobalData::g_trading_day_init_flag_ = true;
        }

        md_record_name_ = md_record_dir_ + '/' + trading_day + "_md";

        if(if_record == 1)
        {
             LOG_INFO(LOGGER, "record file {}", md_record_name_);
        }
        md_record_file_stream_ptr_ = std::make_shared<std::ofstream>(md_record_name_, ios::app|ios::binary);

    }
    else
    {
         LOG_ERROR(LOGGER, "failed to login md error id {}, error msg {}" , pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        {
            std::lock_guard<std::mutex> lg(login_mxt);
            login_flag = false;
        }
        login_cv.notify_one();
    }

}

void CTPMarketDataImpl::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

    /*if (pSpecificInstrument)
    {
        printf("\t>>>InstrumentID = [%s]\n", pSpecificInstrument->InstrumentID);
    }*/
    if (pRspInfo->ErrorID != 0)
    {
        printf(">>>OnRspSubMarketData>[ERR]\n");
        printf("\t>>>ErrorMsg = [%s]\n", pRspInfo->ErrorMsg);
        printf("\t>>>ErrorID = [%d]\n", pRspInfo->ErrorID);
    }
}

bool CTPMarketDataImpl::Login(const std::string username, const std::string password, char** paras) {
    int ret = 0;
    {
        std::unique_lock<std::mutex> ul(connected_mxt);
        if (!connected_cv.wait_for(ul, std::chrono::seconds(10), [this] { return connected; })) {
             LOG_ERROR(LOGGER, "quote front connect failed, timeout");
            return false;
        }
    }

    CThostFtdcReqUserLoginField reqUserLogin{};

    strcpy(reqUserLogin.BrokerID, broker_id_.c_str());
    strcpy(reqUserLogin.UserID, user_id_.c_str());
    strcpy(reqUserLogin.Password, password_.c_str());
    strcpy(reqUserLogin.UserProductInfo, "");

    ret = m_pMdApi->ReqUserLogin(&reqUserLogin, ++m_request_id);
    if (ret != 0)
    {
         LOG_ERROR(LOGGER, "login failed, ret {}", ret);
        return false;
    }
     LOG_INFO(LOGGER, "login ...");
    {
        std::unique_lock<std::mutex> ul(login_mxt);
        if (!login_cv.wait_for(ul, std::chrono::seconds(60), [this] { return login_flag; }))
        {
             LOG_ERROR(LOGGER, "login failed, timeout");
            return false;
        }
    }
     LOG_INFO(LOGGER, "login success");

    return true;
}

int CTPMarketDataImpl::SubscribeMarketData() {
    int inst_count = notice_symbols.size();
    char** ppInstrumentID = new char* [inst_count];
    int i = 0;
    for (const auto& inst : notice_symbols)
    {
        ppInstrumentID[i] = new char[20];
        strcpy(ppInstrumentID[i], inst.c_str());
        i++;
    }


    int ret =  m_pMdApi->SubscribeMarketData(ppInstrumentID, inst_count);
    if (ret != 0)
    {
         LOG_ERROR(LOGGER, "SubscribeMarketData fail");
    }
    return 0;
}

void CTPMarketDataImpl::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) {

//        printf("OnRtnDepthMarketData: %s %s %f\n",
//               pDepthMarketData->ExchangeID, pDepthMarketData->InstrumentID, pDepthMarketData->AveragePrice);
    strcpy(pDepthMarketData->ExchangeID, ExchangeNameConvert(GlobalData::g_symbol_map_[pDepthMarketData->InstrumentID].ExchangeId));
    strcpy(pDepthMarketData->ActionDay, GlobalData::g_trading_day_.c_str());
    strcpy(pDepthMarketData->TradingDay, GlobalData::g_trading_day_.c_str());
    if (if_record && md_record_file_stream_ptr_->is_open())
    {
        md_record_file_stream_ptr_->write(reinterpret_cast<const char *>(pDepthMarketData), sizeof(CThostFtdcDepthMarketDataField));
    }
    auto msg = std::make_shared<CTPTickMessage>(++ctp_tick_num, MessageType::CTPTICKDATA, "ctp_tick");
    msg->set_msg_body(pDepthMarketData);
    PushOrNoticeMsgToAllSubscriber(msg);
}

void CTPMarketDataImpl::SubscribeCTPMD(MessageType type, std::vector<std::string> symbols) {
    if (type==MessageType::CTPTICKDATA)
    {
        notice_symbols = symbols;
        SubscribeMarketData();
    }
    else
    {
         LOG_ERROR(LOGGER, "not my job");
    }
}
