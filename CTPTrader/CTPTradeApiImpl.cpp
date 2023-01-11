//
// Created by zhanghaichao on 2021/12/17.
//

#include "CTPTradeApiImpl.h"


#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <thread>
#include <utility>


bool CTPTradeApiImpl::Init()
{
    std::string ctp_ini_file = "ctp.ini";
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

    }
    else
    {
          LOG_INFO(LOGGER, "test mode");
        account_node = root_node.get_child("simnow");
    }
    if_record =  run_mode.get<int>("record_md");
    if(if_record == 1)
    {
        trade_record_dir_ =  run_mode.get<std::string>("record_dir");
          LOG_INFO(LOGGER, "trade record on, trade dir {}", trade_record_dir_);
    }

    basic_data_base_dir = run_mode.get<string>("basic_data_base_dir");
      LOG_INFO(LOGGER, "basic data base dir", basic_data_base_dir);
    printf("basic data base dir %s\n", basic_data_base_dir.c_str());


    trade_front_ = account_node.get<string>("trade_front");
    quote_front_ = account_node.get<string>("quote_front");
    broker_id_ =account_node.get<string>("broker_id");
    user_id_ = account_node.get<string>("user_id");
    password_ = account_node.get<string>("password");
    app_id_ = account_node.get<string>("app_id");
    auth_code_ = account_node.get<string>("auth_code");
    if_qry_commi_ = account_node.get<int>("qry_commi");
    if_qry_margin_ = account_node.get<int>("qry_margin");
    if_qry_quote_ = account_node.get<int>("qry_quote");
    LOG_DEBUG(LOGGER,"trade_front {}", trade_front_);
    LOG_DEBUG(LOGGER,"quote_front {}", quote_front_);
    LOG_DEBUG(LOGGER,"broker_id {}", broker_id_);
    LOG_DEBUG(LOGGER,"user_id {}", user_id_);
    LOG_DEBUG(LOGGER,"password {}", password_);
    LOG_DEBUG(LOGGER,"app_id {}", app_id_);
    LOG_DEBUG(LOGGER,"auth_code {}", auth_code_);
    LOG_DEBUG(LOGGER,"if qry quote {}", if_qry_commi_);
    LOG_DEBUG(LOGGER,"if qry margin {}", if_qry_margin_);
    LOG_DEBUG(LOGGER,"if qry quote {}", if_qry_quote_);



    auto concern_node = root_node.get_child("concern_inst");
    string concern_insts_str_ = concern_node.get<string>("insts");
    if (concern_insts_str_ != "all")
    {
        boost::split(concern_inst_, concern_insts_str_, boost::is_any_of(","), boost::token_compress_on);
    }
    if (!concern_inst_.empty())
    {
        for (const auto & i : concern_inst_)
        {
              LOG_INFO(LOGGER, "ctp real mode append concern symbol {}", i);
            printf("ctp real mode append concern symbol %s\n", i.c_str());
        }
    }
    else
    {
          LOG_INFO(LOGGER, "ctp real mode concern all symbol");
        printf("ctp real mode concern all symbol\n");
    }


    m_pTradeApi = CThostFtdcTraderApi::CreateFtdcTraderApi();

      LOG_INFO(LOGGER, CThostFtdcTraderApi::GetApiVersion());

    m_pTradeApi->RegisterSpi(this);
    m_pTradeApi->SubscribePrivateTopic(THOST_TERT_QUICK);
    m_pTradeApi->SubscribePublicTopic(THOST_TERT_QUICK);
    m_pTradeApi->RegisterFront(const_cast<char *>(trade_front_.c_str()));
    m_pTradeApi->Init();
    return true;
}

bool CTPTradeApiImpl::Login(const std::string username, const std::string password, char** paras)
{
    int ret = 0;
    {
        std::unique_lock<std::mutex> ul(connected_mxt);
        if (!connected_cv.wait_for(ul, std::chrono::seconds(60), [this] { return connected; })) {
             LOG_ERROR(LOGGER, "trade front connect failed, timeout");
            printf("trade front connect failed, timeout\n");
            return false;
        }
    }

    CThostFtdcReqUserLoginField reqUserLogin{};

    strcpy(reqUserLogin.BrokerID, broker_id_.c_str());
    strcpy(reqUserLogin.UserID, user_id_.c_str());
    strcpy(reqUserLogin.Password, password_.c_str());
    strcpy(reqUserLogin.UserProductInfo, "");

    ret = m_pTradeApi->ReqUserLogin(&reqUserLogin, ++m_request_id);
    if (ret != 0)
    {
         LOG_ERROR(LOGGER, "login failed, ret {}", ret);
        return false;
    }
      LOG_INFO(LOGGER, "login ...");
    {
        std::unique_lock<std::mutex> ul(login_mxt);
        if (!login_cv.wait_for(ul, std::chrono::seconds(10), [this] { return login_flag; }))
        {
             LOG_ERROR(LOGGER, "login failed, timeout");
            return false;
        }
    }
      LOG_INFO(LOGGER, "login success");

    // do something init
    // 1. confirm settlefile
    CThostFtdcSettlementInfoConfirmField confirmField{};
    strcpy(confirmField.BrokerID, broker_id_.c_str());
    strcpy(confirmField.InvestorID, user_id_.c_str());
    ret = m_pTradeApi->ReqSettlementInfoConfirm(&confirmField, ++m_request_id);

    // 2.1 query fund
    CThostFtdcQryTradingAccountField accountField{};
    strcpy(accountField.BrokerID, broker_id_.c_str());
    strcpy(accountField.InvestorID, user_id_.c_str());
    ret = m_pTradeApi->ReqQryTradingAccount(&accountField, ++m_request_id);
    {
        std::unique_lock<std::mutex> ul(qry_mtx_);
        if (!qry_cv_.wait_for(ul, std::chrono::seconds(10), [this] {return qry_flag_;}))
        {
             LOG_ERROR(LOGGER, "query fund timeout");
            return false;
        }
    }

    qry_flag_ = false;
    // 2.2 query order
    CThostFtdcQryOrderField qryOrderField{};
    strcpy(qryOrderField.BrokerID, broker_id_.c_str());
    strcpy(qryOrderField.InvestorID, user_id_.c_str());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ret = m_pTradeApi->ReqQryOrder(&qryOrderField, ++m_request_id);
    {
        std::unique_lock<std::mutex> ul(qry_mtx_);
        if (!qry_cv_.wait_for(ul, std::chrono::seconds(10), [this] {return qry_flag_;}))
        {
             LOG_ERROR(LOGGER, "query order timeout");
            return false;
        }
    }

    qry_flag_ = false;
    // 2.3 query trade
    CThostFtdcQryTradeField qryTradeField{};
    strcpy(qryTradeField.BrokerID, broker_id_.c_str());
    strcpy(qryTradeField.InvestorID, user_id_.c_str());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ret = m_pTradeApi->ReqQryTrade(&qryTradeField, ++m_request_id);
    {
        std::unique_lock<std::mutex> ul(qry_mtx_);
        if (!qry_cv_.wait_for(ul, std::chrono::seconds(10), [this] {return qry_flag_;}))
        {
             LOG_ERROR(LOGGER, "query trade timeout");
            return false;
        }
    }

    qry_flag_ = false;
    // 2.4 query position
    CThostFtdcQryInvestorPositionField qryInvestorPositionField{};
    strcpy(qryInvestorPositionField.BrokerID, broker_id_.c_str());
    strcpy(qryInvestorPositionField.InvestorID, user_id_.c_str());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ret = m_pTradeApi->ReqQryInvestorPosition(&qryInvestorPositionField, ++m_request_id);
    {
        std::unique_lock<std::mutex> ul(qry_mtx_);
        if (!qry_cv_.wait_for(ul, std::chrono::seconds(10), [this] {return qry_flag_;}))
        {
             LOG_ERROR(LOGGER, "query position timeout");
            return false;
        }
    }

    // init instrument
    qry_flag_ = false;
    // 3.query instrument
    CThostFtdcQryInstrumentField qryInstrumentField{};

    std::this_thread::sleep_for(std::chrono::seconds(1));
    ret = m_pTradeApi->ReqQryInstrument(&qryInstrumentField, m_request_id++);
    {
        std::unique_lock<std::mutex> ul(qry_mtx_);
        if (!qry_cv_.wait_for(ul, std::chrono::seconds(60), [this] {return qry_flag_;}))
        {
             LOG_ERROR(LOGGER, "query instrument timeout");
            return false;
        }
    }
    printf("query symbol finish\n");

    // 3.2 init symbol index
    GlobalData::InitSymbolIndex();
    GlobalData::g_symbol_init_flag_ = true;

    for_each(inst_map_.begin(), inst_map_.end(), [](std::pair<std::string, CThostFtdcInstrumentField> i)
    {
        printf("symbol: %s %s %f %d\n", i.first.c_str(), i.second.ExchangeID, i.second.PriceTick, i.second.VolumeMultiple);
    });

    qry_flag_ = false;

    //
      LOG_INFO(LOGGER, "start init instruments margin, commi, quote");

    // 4.1
    if (if_qry_commi_)
    {
        for (const auto& inst : inst_map_)
        {

            // query commi rate field
            CThostFtdcQryInstrumentCommissionRateField qryInstrumentCommissionRateField{};
            strcpy(qryInstrumentCommissionRateField.BrokerID, broker_id_.c_str());
            strcpy(qryInstrumentCommissionRateField.InvestorID, user_id_.c_str());

            strcpy(qryInstrumentCommissionRateField.InstrumentID, inst.first.c_str());
            m_pTradeApi->ReqQryInstrumentCommissionRate(&qryInstrumentCommissionRateField, m_request_id++);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    else
    {
        printf("init commi from bin file\n");
        InitCommiFromBinFile();
    }

    // 4.2
    if (if_qry_margin_)
    {
        for (const auto& inst : inst_map_)
        {
            // query margin rate field
            CThostFtdcQryInstrumentMarginRateField qryInstrumentMarginRateField{};
            strcpy(qryInstrumentMarginRateField.BrokerID, broker_id_.c_str());
            strcpy(qryInstrumentMarginRateField.InvestorID, user_id_.c_str());
            qryInstrumentMarginRateField.HedgeFlag = THOST_FTDC_HF_Speculation;

            strcpy(qryInstrumentMarginRateField.InstrumentID, inst.first.c_str());
            m_pTradeApi->ReqQryInstrumentMarginRate(&qryInstrumentMarginRateField, m_request_id++);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    else
    {
        printf("init margin from bin file\n");
        InitMarginFromBinFile();
    }

    // 4.3
    if (if_qry_quote_)
    {
        for (const auto& inst : inst_map_)
        {
            // query market data field
            CThostFtdcQryDepthMarketDataField qryDepthMarketDataField{};
            strcpy(qryDepthMarketDataField.InstrumentID, inst.first.c_str());
            m_pTradeApi->ReqQryDepthMarketData(&qryDepthMarketDataField, m_request_id++);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

    }
    else
    {
        printf("init market from bin file\n");
        InitMarketFromBinFile();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

      LOG_INFO(LOGGER, "concern_inst_ {}, commi_map_ {}, margin_map_ {}, market_map_ {}",
                          concern_inst_.size(), commi_map_.size(), market_map_.size(), market_map_.size());

    InitMainSymbol();

    if (if_record)
    {
        if (!inst_map_.empty())
        {
            auto inst_file_ = trade_record_dir_ + '/' + GlobalData::g_trading_day_ + "_inst";
              LOG_INFO(LOGGER, "record inst {}, size {}", inst_file_, inst_map_.size());
            std::ofstream inst_file_stream_{inst_file_, ios::binary|ios::out|ios::trunc};
            for (auto & i : inst_map_)
            {
                inst_file_stream_.write((char *)(&(i.second)), sizeof(i.second));
            }
        }
        if (if_qry_commi_ && !commi_map_.empty())
        {
            auto commi_file_ = trade_record_dir_ + '/' + GlobalData::g_trading_day_ + "_commi";
              LOG_INFO(LOGGER, "record commi {}, size {}", commi_file_, commi_map_.size());
            std::ofstream commi_file_stream_{commi_file_, ios::binary|ios::out|ios::trunc};
            for (auto & i : commi_map_)
            {
                commi_file_stream_.write((char *)(&(i.second)), sizeof(i.second));
            }
        }
        if (if_qry_margin_ && ! margin_map_.empty())
        {
            auto margin_file_ = trade_record_dir_ + '/' + GlobalData::g_trading_day_ + "_margin";
              LOG_INFO(LOGGER, "record margin {}, size {}", margin_file_, margin_file_.size());
            std::ofstream margin_file_stream_{margin_file_, ios::binary|ios::out|ios::trunc};
            for (auto & i : margin_map_)
            {
                margin_file_stream_.write((char *)(&(i.second)), sizeof(i.second));
            }
        }
        if (if_qry_quote_ && ! market_map_.empty())
        {
            auto market_file_ = trade_record_dir_ + '/' + GlobalData::g_trading_day_ + "_market";
              LOG_INFO(LOGGER, "record market {}, size {}", market_file_, market_map_.size());
            std::ofstream market_file_stream_{market_file_, ios::binary|ios::out|ios::trunc};
            for (auto & i : market_map_)
            {
                market_file_stream_.write((char *)(&(i.second)), sizeof(i.second));
            }
        }
    }

    return true;
}




void CTPTradeApiImpl::OnFrontConnected(void)
{
    printf("OnFrontConnected\n");
      LOG_INFO(LOGGER, "OnFrontConnected");
    CThostFtdcReqAuthenticateField field{};

    strcpy(field.BrokerID, broker_id_.c_str());
    strcpy(field.UserID, user_id_.c_str());

    strcpy(field.AuthCode, auth_code_.c_str());
    strcpy(field.AppID, app_id_.c_str());

    if (is_real == 0)
    {
        {
            std::lock_guard<std::mutex> lg(connected_mxt);
            connected = true;
        }
        connected_cv.notify_one();
        return;
    }

    m_pTradeApi->ReqAuthenticate(&field, ++m_request_id);

}

void CTPTradeApiImpl::OnFrontDisconnected(int nReason)
{
    LOG_DEBUG(LOGGER,"OnFrontDisconnected");
    {
        std::lock_guard<std::mutex> lg(connected_mxt);
        connected = false;
    }
    connected_cv.notify_one();
}

void CTPTradeApiImpl::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField,
                                            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    printf("OnRspAuthenticate\n");
    if (pRspInfo != nullptr && pRspInfo->ErrorID != 0)
    {
         LOG_ERROR(LOGGER, "OnRspAuthenticate notify, error id {}, error msg {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        {
            std::lock_guard<std::mutex> lg(connected_mxt);
            connected = false;
        }
        connected_cv.notify_one();

    }
    else
    {
          LOG_INFO(LOGGER, "OnRspAuthenticate successfully");
        {
            std::lock_guard<std::mutex> lg(connected_mxt);
            connected = true;
        }
        connected_cv.notify_one();
    }
}

void CTPTradeApiImpl::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo,
                                         int nRequestID, bool bIsLast)
{
    printf("OnRspUserLogin\n");
    if (pRspInfo->ErrorID == 0)
    {
        {
            std::lock_guard<std::mutex> lg(login_mxt);
            login_flag = true;
        }
        login_cv.notify_one();

        front_id_ = pRspUserLogin->FrontID;
        session_id_ = pRspUserLogin->SessionID;
        trading_day_ = pRspUserLogin->TradingDay;
        max_order_ref_ = pRspUserLogin->MaxOrderRef;

          LOG_INFO(LOGGER, "user login, trading day {},MaxOrderRef {}, FrontID {}, SessionID {}" ,
                              trading_day_, max_order_ref_, front_id_, session_id_);
        GlobalData::g_trading_day_ = trading_day_;
        GlobalData::g_trading_day_init_flag_ = true;

    }
    else
    {
         LOG_ERROR(LOGGER, "failed to login error id {}, error msg {}" , pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        {
            std::lock_guard<std::mutex> lg(login_mxt);
            login_flag = false;
        }
        login_cv.notify_one();
    }
}

std::shared_ptr<OrderField>  CTPTradeApiImpl::InsertLimitPriceOrder(const std::string symbol, OrderDirection direction,
                                               OrderOffsetFlag offsetFlag, int volume, double price, std::shared_ptr<StrategyBase> strategy)
{
    auto order = std::make_shared<OrderField>();
    order->OrderID = GetOrderID();
    strcpy(order->Symbol, symbol.c_str());
    order->Direction = direction;
    order->OffsetFlag = offsetFlag;
    order->Volume = volume;
    order->Price = price;
    order->strategy = strategy;

    CThostFtdcInputOrderField ctpOrderField{};

    strcpy(ctpOrderField.InstrumentID, symbol.c_str());

    auto exchange = GlobalData::GetSymbolField(symbol).ExchangeId;
    strcpy(ctpOrderField.ExchangeID, ExchangeNameConvert(exchange));

    ctpOrderField.LimitPrice = price;
    ctpOrderField.VolumeTotalOriginal = volume;

    if (direction == BUY)
        ctpOrderField.Direction = THOST_FTDC_D_Buy;
    else
        ctpOrderField.Direction = THOST_FTDC_D_Sell;

    if (offsetFlag == OPEN)
        ctpOrderField.CombOffsetFlag[0] = THOST_FTDC_OFEN_Open;
    else if (exchange == ExchangeID::SHFE || exchange == ExchangeID::INE)
        ctpOrderField.CombOffsetFlag[0] = THOST_FTDC_OFEN_CloseToday;
    else
        ctpOrderField.CombOffsetFlag[0] = THOST_FTDC_OFEN_Close;

    ctpOrderField.RequestID = GetCtpOrderID();

    strcpy(ctpOrderField.BrokerID, broker_id_.c_str());
    strcpy(ctpOrderField.UserID, user_id_.c_str());
    strcpy(ctpOrderField.InvestorID, user_id_.c_str());


    ctpOrderField.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    ctpOrderField.TimeCondition = THOST_FTDC_TC_GFD;

    ctpOrderField.CombHedgeFlag[0] = THOST_FTDC_HFEN_Speculation;

    ctpOrderField.VolumeCondition = THOST_FTDC_VC_AV;
    ctpOrderField.ContingentCondition = THOST_FTDC_CC_Immediately;
    ctpOrderField.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;



    int ret = m_pTradeApi->ReqOrderInsert(&ctpOrderField, ctpOrderField.RequestID);
    if (ret != 0)
    {
        order->Status = OrderStatus::REJECTED;
         LOG_ERROR(LOGGER, "strategy %s InsertLimitPriceOrder failed: order id {}, ctp request id {}, symbol {}, direction {}, offset {}, "
                      "price {}, volume {}, status {}",
                               order->strategy.lock()->strategy_name, order->OrderID, ctpOrderField.RequestID, order->Symbol, order->Direction, order->OffsetFlag,
                               order->Price, order->Volume, order->Status);
        return nullptr;
    }
    order->Status = OrderStatus::UNKNOWN;

    AppendOrderIDMap(ctpOrderField.RequestID, order->OrderID);
    AppendNewOrder(order->OrderID, order);

    LOG_DEBUG(LOGGER,"InsertLimitPriceOrder success: strategy {} ,order id {}, ctp request id {}, symbol {}, direction {}, offset {}, "
                  "price {}, volume {}, status {}",
                           order->strategy.lock()->strategy_name, order->OrderID, ctpOrderField.RequestID, order->Symbol, order->Direction, order->OffsetFlag,
                           order->Price, order->Volume, order->Status);

    return order;
}

bool CTPTradeApiImpl::CancelCtpOrder(std::string exchange_id, std::string order_sys_id, int order_id, std::string symbol) {

    CThostFtdcInputOrderActionField actionField{};
    actionField.ActionFlag = THOST_FTDC_AF_Delete;
    actionField.FrontID = front_id_;
    actionField.SessionID = session_id_;

    auto it = order_id_ctp_order_ref_map_.find(order_id);
    if (it != order_id_ctp_order_ref_map_.end())
        strcpy(actionField.OrderRef, it->second.c_str());

    strcpy(actionField.ExchangeID, exchange_id.c_str());
    strcpy(actionField.OrderSysID, order_sys_id.c_str());


    strcpy(actionField.BrokerID, broker_id_.c_str());
    strcpy(actionField.UserID, user_id_.c_str());
    strcpy(actionField.InstrumentID, symbol.c_str());
    strcpy(actionField.InvestorID, user_id_.c_str());

    actionField.RequestID = GetOrderID();

    auto ret = m_pTradeApi->ReqOrderAction(&actionField, actionField.RequestID);
    if (ret)
    {
        return true;
    }
    return false;
}

std::shared_ptr<OrderField>
CTPTradeApiImpl::InsertStopOrder(const std::string symbol, OrderDirection direction, OrderOffsetFlag offsetFlag,
                                     int volume, double price, std::shared_ptr<StrategyBase> strategy) {
    auto order = std::make_shared<OrderField>();
    return order;
}

void CTPTradeApiImpl::OnRspOrderInsert(CThostFtdcInputOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo,
                                           int nRequestID, bool bIsLast) {
    if (pRspInfo != nullptr && pRspInfo->ErrorID == 0)
    {
        if (pOrder != nullptr)
        {
            auto order_id = GetOrderIDByCtpRequestID(nRequestID);
            if (order_id != -1)
            {
                auto order = GetOrderByID(order_id);
                if (order)
                {
                    order->Status = OrderStatus::INSERTED;

                    traderCallBack->OnRspOrderInsert(order);

                      LOG_INFO(LOGGER, "OnRspOrderInsert success: strategy name {}, ctp order request id{}, order id {}, order ref {}, "
                                 "symbol {}, direction {}, offset {}, price {}, volume {}",
                                          order->strategy.lock()->strategy_name, pOrder->RequestID, order->OrderID, pOrder->OrderRef, order->Symbol, order->Direction,
                                          order->OffsetFlag, order->Price, order->Volume);
                }
            }
            else
            {
                 LOG_ERROR(LOGGER, "OnRspOrderInsert failed: ctp request id not find {}", nRequestID);
            }
        }
    }
    else if (pRspInfo != nullptr)
    {
        printf("OnRspOrderInsert failed %d %s, request id: %d\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID);
         LOG_ERROR(LOGGER, "OnRspOrderInsert failed {} {}, request id {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID);
    }

}

void
CTPTradeApiImpl::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo) {
    if (pRspInfo!= nullptr && pInputOrder!= nullptr)
    {
         LOG_ERROR(LOGGER, "OnErrRtnOrderInsert {} {}, ctp request id {}",
                               pRspInfo->ErrorID, pRspInfo->ErrorMsg, pInputOrder->RequestID);

        auto order_id = GetOrderIDByCtpRequestID(pInputOrder->RequestID);
        if (order_id != -1)
        {
            auto order = GetOrderByID(order_id);
            if (order)
            {
                order->Status = OrderStatus::REJECTED;

                  LOG_INFO(LOGGER, "OnRspOrderInsert failed: strategy name {}, ctp order request id{}, order id {}, order ref {}, "
                             "symbol {}, direction {}, offset {}, price {}, volume {}",
                                      order->strategy.lock()->strategy_name, pInputOrder->RequestID, order->OrderID, pInputOrder->OrderRef, order->Symbol, order->Direction,
                                      order->OffsetFlag, order->Price, order->Volume);

                traderCallBack->OnRspOrderInsert(order);
            }
        }
        else
        {
             LOG_ERROR(LOGGER, "OnRspOrderInsert failed: ctp request id not find {}", pInputOrder->RequestID);
        }

    }
}

void CTPTradeApiImpl::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
     LOG_ERROR(LOGGER, "OnRspError {} {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
}

void CTPTradeApiImpl::OnRtnOrder(CThostFtdcOrderField *pOrder) {
    if (pOrder
       && pOrder->FrontID == front_id_
       && pOrder->SessionID == session_id_ )
    {

        CThostFtdcOrderField orderField{};
        memcpy(&orderField, pOrder, sizeof(orderField));
        auto it = ctp_order_map_.find(orderField.InstrumentID);
        if ( it == ctp_order_map_.end())
        {
            std::vector<CThostFtdcOrderField> orders_;
            orders_.push_back(orderField);
            ctp_order_map_.emplace(orderField.InstrumentID, orders_);
        }
        else
        {
            it->second.push_back(orderField);
        }


        auto order_id = GetOrderIDByCtpRequestID(pOrder->RequestID);
        if (order_id != -1)
        {
            auto order = GetOrderByID(order_id);
            if (order)
            {
                order->Status = CTPOrderStatusConvert(pOrder->OrderStatus);
                if ( strlen(order->OrderRef) == 0)
                {
                    strcpy(order->OrderRef, pOrder->OrderSysID);
                    AppendOrderSysIDMap(order->OrderRef, order->OrderID);
                }

                if (order->VolumeTraded < pOrder->VolumeTraded)
                    order->VolumeTraded = pOrder->VolumeTraded;
                if (order->Status == OrderStatus::ACCEPTED)
                    strcpy(order->AcceptedTime, pOrder->UpdateTime);

                strcpy(order->InsertTime, pOrder->InsertTime);
                strcpy(order->UpdateTime, pOrder->UpdateTime);

                order_id_ctp_order_ref_map_[order_id] = pOrder->OrderRef;

                  LOG_INFO(LOGGER, "CTP OnRtnOrder: strategy name {}, symbol {}, ctp request id {}, order id {}, ctp order ref {}, ctp order sys id {}, direction {}, "
                             "offset {}, price {}, total volume original {}, volume traded {}, status {}, "
                             "insert time {}, ctp order status {}, "
                             "ctp status msg {}",
                                      order->strategy.lock()->strategy_name, order->Symbol, pOrder->RequestID, order->OrderID, pOrder->OrderRef, order->OrderRef, order->Direction,
                                      order->OffsetFlag, order->Price, order->Volume, order->VolumeTraded, order->Status,
                                      pOrder->InsertTime, pOrder->OrderStatus, pOrder->StatusMsg);

//                printf("CTP OnRtnOrder: strategy name %s, symbol %s, ctp request id %d, order id %d, ctp order ref %s, "
//                       "ctp order sys id %s, direction %s, "
//                       "offset %s, price %.2lf, total volume original %d, volume traded %d, status %d, "
//                       "insert time %s, ctp order status %d, ctp status msg %s\n",
//                       order->strategy.lock()->strategy_name.c_str(), order->Symbol, pOrder->RequestID, order->OrderID, pOrder->OrderRef,
//                       order->OrderRef,
//                       order->Direction == OrderDirection::BUY ? "Buy" : "Sell",
//                       (order->OffsetFlag == OrderOffsetFlag::OPEN ) ? "Open" : order->OffsetFlag == OrderOffsetFlag::CLOSE ? "Close" : "CloseYesterday",
//                       order->Price, order->Volume, order->VolumeTraded, order->Status,
//                       pOrder->InsertTime, pOrder->OrderStatus, pOrder->StatusMsg);

                traderCallBack->OnRtnOrder(order);
            }
        }
        else
        {
             LOG_ERROR(LOGGER, "CTP OnRspOrderInsert failed: ctp request id not find {}", pOrder->RequestID);
        }


    }
    else            /// TODO other order
    {

    }
}


void CTPTradeApiImpl::OnRtnTrade(CThostFtdcTradeField *pTrade) {
    if (pTrade)
    {
        auto order_id = GetOrderIDByCtpOrderSysID(pTrade->OrderSysID);
        if (order_id != -1)
        {
            auto order = GetOrderByID(order_id);
            if (order)
            {
                auto trade = std::make_shared<TradeField>();
                trade->OrderID = order->OrderID;
                strcpy(trade->Symbol, order->Symbol);
                trade->Direction = order->Direction;
                trade->OffsetFlag = order->OffsetFlag;
                trade->TradeVolume = pTrade->Volume;
                trade->TradePrice = pTrade->Price;
                strcpy(trade->TradeID, pTrade->TradeID);
                strcpy(trade->OrderRef, pTrade->OrderSysID);
                strcpy(trade->TradeTime, pTrade->TradeTime);

                  LOG_INFO(LOGGER, "CTP OnRtnTrade: strategy name {}, {}, order id {}, ctp order sys id {}, trade id {}, "
                             "Direction {}, OffsetFlag {}, Price {}, Volume {}, TradeTime{}",
                                      order->strategy.lock()->strategy_name, trade->Symbol, trade->OrderID, trade->OrderRef, trade->TradeID,
                                      trade->Direction, trade->OffsetFlag, trade->TradePrice, trade->TradeVolume, pTrade->TradeTime);
//                printf("CTP OnRtnTrade: strategy name %s, symbol %s, order id %d, "
//                       "ctp order sys id %s, "
//                       "direction %s, "
//                       "offset %s, "
//                       "price %.2lf, total volume original %d, volume traded %d, status %d\n",
//                       order->strategy.lock()->strategy_name.c_str(), order->Symbol, order->OrderID,
//                       order->OrderRef,
//                       order->Direction == OrderDirection::BUY ? "Buy" : "Sell",
//                       (order->OffsetFlag == OrderOffsetFlag::OPEN ) ? "Open" : order->OffsetFlag == OrderOffsetFlag::CLOSE ? "Close" : "CloseYesterday",
//                       order->Price, order->Volume, order->VolumeTraded, order->Status);
                traderCallBack->OnRtnTrade(order, trade);
            }
        }
        else
        {
             LOG_ERROR(LOGGER, "OnRtnTrade failed: ctp order sys id not find {}", pTrade->OrderSysID);
        }
    }

}

void
CTPTradeApiImpl::OnRspQryClassifiedInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo,
                                              int nRequestID, bool bIsLast) {

    CThostFtdcInstrumentField inst{};
    if (pInstrument)
    {
        memcpy(&inst, pInstrument, sizeof(inst));

        if (concern_inst_.empty() ||
            concern_inst_.find(pInstrument->InstrumentID) != concern_inst_.end())
        {
            inst_map_.emplace(inst.InstrumentID, inst);
            SymbolField symbolField{};
            strcpy(symbolField.Symbol, inst.InstrumentID);
            symbolField.ExchangeId = ExchangeIDConvert(inst.ExchangeID);
            symbolField.PriceTick = inst.PriceTick;
            symbolField.Multiple = inst.VolumeMultiple;
            symbolField.MaxMarketOrderVolume = inst.MaxMarketOrderVolume;
            symbolField.MinMarketOrderVolume = inst.MinMarketOrderVolume;
            symbolField.MaxLimitOrderVolume = inst.MaxLimitOrderVolume;
            symbolField.MinLimitOrderVolume = inst.MinLimitOrderVolume;
            strcpy(symbolField.ProductID, inst.ProductID);
            GlobalData::g_symbol_map_.emplace(inst.InstrumentID, symbolField);
            GlobalData::g_symbol_trading_end_map_.emplace(inst.InstrumentID, true);
        }
    }

    if (bIsLast)
    {
          LOG_INFO(LOGGER, "finish query instruments, total instruments : {}", inst_map_.size());
        {
            std::unique_lock<std::mutex> ul(qry_mtx_);
            qry_flag_ = true;
//            GlobalData::g_symbol_init_flag_ = true;
        }
        qry_cv_.notify_all();
    }
}

void CTPTradeApiImpl::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo,
                                         int nRequestID, bool bIsLast) {
    LOG_DEBUG(LOGGER,"OnRspQryInstrument {}", pInstrument->InstrumentID);
    if (pInstrument->ProductClass == THOST_FTDC_PC_Futures && pInstrument->IsTrading == true)
    {
        CThostFtdcInstrumentField inst{};
        memcpy(&inst, pInstrument, sizeof(inst));
        if (concern_inst_.empty() ||
            concern_inst_.find(pInstrument->InstrumentID) != concern_inst_.end())
        {
            inst_map_.emplace(inst.InstrumentID, inst);
            SymbolField symbolField{};
            strcpy(symbolField.Symbol, inst.InstrumentID);
            symbolField.ExchangeId = ExchangeIDConvert(inst.ExchangeID);
            symbolField.PriceTick = inst.PriceTick;
            symbolField.Multiple = inst.VolumeMultiple;
            symbolField.MaxMarketOrderVolume = inst.MaxMarketOrderVolume;
            symbolField.MinMarketOrderVolume = inst.MinMarketOrderVolume;
            symbolField.MaxLimitOrderVolume = inst.MaxLimitOrderVolume;
            symbolField.MinLimitOrderVolume = inst.MinLimitOrderVolume;
            strcpy(symbolField.ProductID, inst.ProductID);
            GlobalData::g_symbol_map_.emplace(inst.InstrumentID, symbolField);
            GlobalData::g_symbol_trading_end_map_.emplace(inst.InstrumentID, true);
        }
    }

    if (bIsLast)
    {
          LOG_INFO(LOGGER, "finish query instruments, total instruments : {}", inst_map_.size());
        {
            std::unique_lock<std::mutex> ul(qry_mtx_);
            qry_flag_ = true;
        }
        qry_cv_.notify_all();
    }
}

void CTPTradeApiImpl::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate,
                                                   CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    LOG_DEBUG(LOGGER,"OnRspQryInstrumentMarginRate {}", pInstrumentMarginRate->InstrumentID);
    CThostFtdcInstrumentMarginRateField margin{};
    memcpy(&margin, pInstrumentMarginRate, sizeof(margin));
    margin_map_.emplace(margin.InstrumentID, margin);

}

void
CTPTradeApiImpl::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate,
                                                  CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    LOG_DEBUG(LOGGER,"OnRspQryInstrumentCommissionRate {}", pInstrumentCommissionRate->InstrumentID);
    CThostFtdcInstrumentCommissionRateField commi{};
    memcpy(&commi, pInstrumentCommissionRate, sizeof(commi));
    commi_map_.emplace(commi.InstrumentID, commi);
}

void CTPTradeApiImpl::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData,
                                              CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    LOG_DEBUG(LOGGER,"OnRspQryDepthMarketData {}", pDepthMarketData->InstrumentID);

    if (inst_map_.find(pDepthMarketData->InstrumentID) != inst_map_.end())
    {
        CThostFtdcDepthMarketDataField market{};
        memcpy(&market, pDepthMarketData, sizeof(market));
        market_map_.emplace(market.InstrumentID, market);
    }
}

void CTPTradeApiImpl::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount,
                                             CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    memcpy(&account_info_, pTradingAccount, sizeof(account_info_));
    if (bIsLast)
    {
          LOG_INFO(LOGGER, "finish query account, available {}", account_info_.Available);
        {
            std::unique_lock<std::mutex> ul(qry_mtx_);
            qry_flag_ = true;
        }
        qry_cv_.notify_all();
    }
}

void CTPTradeApiImpl::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                    bool bIsLast) {
    if(pOrder != nullptr)
    {
        CThostFtdcOrderField orderField{};
        memcpy(&orderField, pOrder, sizeof(orderField));
        auto it = ctp_order_map_.find(orderField.InstrumentID);
        if ( it == ctp_order_map_.end())
        {
            std::vector<CThostFtdcOrderField> orders_;
            orders_.push_back(orderField);
            ctp_order_map_.emplace(orderField.InstrumentID, orders_);
        }
        else
        {
            it->second.push_back(orderField);
        }
    }

    if (bIsLast)
    {
          LOG_INFO(LOGGER, "finish query order");
        {
            for (const auto & os : ctp_order_map_)
            {
                for (const auto & o : os.second)
                {
                      LOG_INFO(LOGGER, "{}, RequestID {}, OrderSysID {}, Direction {}, OffsetFlag {}, "
                                 "Price {}, Volume {}, VolumeTraded {}, OrderStatus {}",
                                          o.InstrumentID,
                                          o.RequestID,
                                          o.OrderSysID,
                                          o.Direction,
                                          o.CombOffsetFlag[0],
                                          o.LimitPrice,
                                          o.VolumeTotalOriginal,
                                          o.VolumeTraded,
                                          o.OrderStatus);
                }
            }
            std::unique_lock<std::mutex> ul(qry_mtx_);
            qry_flag_ = true;
        }
        qry_cv_.notify_all();
    }
}

void CTPTradeApiImpl::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                    bool bIsLast) {

    if (pTrade!= nullptr)
    {
        CThostFtdcTradeField tradeField{};
        memcpy(&tradeField, pTrade, sizeof(tradeField));
        auto it = ctp_trade_map_.find(tradeField.InstrumentID);
        if ( it == ctp_trade_map_.end())
        {
            std::vector<CThostFtdcTradeField> trades_;
            trades_.push_back(tradeField);
            ctp_trade_map_.emplace(tradeField.InstrumentID, trades_);
        }
        else
        {
            it->second.push_back(tradeField);
        }
    }

    if (bIsLast)
    {
          LOG_INFO(LOGGER, "finish query trade");
        {
            for (const auto & ts : ctp_trade_map_)
            {
                for (const auto & t : ts.second)
                {
                      LOG_INFO(LOGGER, "{}, TradeID {}, OrderRef {}, OrderSysID {}, Direction {}, OffsetFlag {},"
                                 "Price {}, Volume {}, TradeTime {}, OrderLocalID {}",
                                          t.InstrumentID,
                                          t.TradeID,
                                          t.OrderRef,
                                          t.OrderSysID,
                                          t.Direction,
                                          t.OffsetFlag,
                                          t.Price,
                                          t.Volume,
                                          t.TradeTime,
                                          t.OrderLocalID);
                }
            }
            std::unique_lock<std::mutex> ul(qry_mtx_);
            qry_flag_ = true;
        }
        qry_cv_.notify_all();
    }
}

void CTPTradeApiImpl::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition,
                                               CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pInvestorPosition != nullptr)
    {
        CThostFtdcInvestorPositionField positionField{};
        memcpy(&positionField, pInvestorPosition, sizeof(positionField));
        auto it = position_map_.find(positionField.InstrumentID);
        if ( it == position_map_.end())
        {
            std::vector<CThostFtdcInvestorPositionField> long_positions_;
            std::vector<CThostFtdcInvestorPositionField> short_positions_;
            auto positions_ = std::make_pair(long_positions_, short_positions_);
            if (positionField.PosiDirection == THOST_FTDC_PD_Long)
                long_positions_.push_back(positionField);
            else if (positionField.PosiDirection == THOST_FTDC_PD_Short)
                short_positions_.push_back(positionField);

            position_map_.emplace(positionField.InstrumentID, positions_);
        }
        else
        {
            if (positionField.PosiDirection == THOST_FTDC_PD_Long)              // 多头持仓
                it->second.first.push_back(positionField);
            else if (positionField.PosiDirection == THOST_FTDC_PD_Short)        // 空头持仓
                it->second.second.push_back(positionField);
        }
    }

    if (bIsLast)
    {
          LOG_INFO(LOGGER, "finish query position");
        {
            for (const auto & ps : position_map_)
            {
                for (const auto & p : ps.second.first)
                {
                      LOG_INFO(LOGGER, "{} long position, YdPosition {}, TodayPosition {}, Position {}, OpenVolume {}, CloseVolume {}, Commission {},"
                                 "CloseProfit {}, PositionProfit {}, OpenCost {}",
                                          p.InstrumentID,
                                          p.YdPosition,
                                          p.TodayPosition,
                                          p.Position,
                                          p.OpenVolume,
                                          p.CloseVolume,
                                          p.Commission,
                                          p.CloseProfit,
                                          p.PositionProfit,
                                          p.OpenCost);
                    printf("%s long position, YdPosition %d, TodayPosition %d, Position %d, OpenVolume %d, CloseVolume %d, Commission %f,"
                           "CloseProfit %f, PositionProfit %f, OpenCost %f\n",
                           p.InstrumentID,
                           p.YdPosition,
                           p.TodayPosition,
                           p.Position,
                           p.OpenVolume,
                           p.CloseVolume,
                           p.Commission,
                           p.CloseProfit,
                           p.PositionProfit,
                           p.OpenCost);
                }
                for (const auto & p : ps.second.second)
                {
                      LOG_INFO(LOGGER, "{} short position, YdPosition {}, TodayPosition {}, Position {}, OpenVolume {}, CloseVolume {}, Commission {},"
                                 "CloseProfit {}, PositionProfit {}, OpenCost {}",
                                          p.InstrumentID,
                                          p.YdPosition,
                                          p.TodayPosition,
                                          p.Position,
                                          p.OpenVolume,
                                          p.CloseVolume,
                                          p.Commission,
                                          p.CloseProfit,
                                          p.PositionProfit,
                                          p.OpenCost);
                    printf("%s short position, YdPosition %d, TodayPosition %d, Position %d, OpenVolume %d, CloseVolume %d, Commission %f,"
                           "CloseProfit %f, PositionProfit %f, OpenCost %f\n",
                           p.InstrumentID,
                           p.YdPosition,
                           p.TodayPosition,
                           p.Position,
                           p.OpenVolume,
                           p.CloseVolume,
                           p.Commission,
                           p.CloseProfit,
                           p.PositionProfit,
                           p.OpenCost);
                }
            }
            std::unique_lock<std::mutex> ul(qry_mtx_);
            qry_flag_ = true;
        }
        qry_cv_.notify_all();
    }
}

void
CTPTradeApiImpl::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) {
    CThostFtdcTraderSpi::OnRspOrderAction(pInputOrderAction, pRspInfo, nRequestID, bIsLast);
}

void CTPTradeApiImpl::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo) {
    CThostFtdcTraderSpi::OnErrRtnOrderAction(pOrderAction, pRspInfo);
}



