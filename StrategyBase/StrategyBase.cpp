//
// Created by zhanghaichao on 2021/12/20.
//

//#include "../Monitor/StrategyPositionMonitor.h"

#include <boost/property_tree/xml_parser.hpp>
#include <utility>
#include "../Message/BarMessage.h"
#include "../Message/CTPTickMessage.h"

#include "StrategyBase.h"
#include "boost/filesystem.hpp"
#include "unistd.h"
#include "logger/LogManager.h"


std::unordered_map<std::string, SymbolField> GlobalData::g_symbol_map_;
std::atomic<bool> GlobalData::g_symbol_init_flag_;
std::string GlobalData::g_trading_day_;
std::atomic<bool> GlobalData::g_trading_day_init_flag_;
ProductTradingPeriod GlobalData::g_product_trading_periods_;
std::vector<size_t> GlobalData::g_day_miute_vector_;
std::unordered_map<int, size_t> GlobalData::g_minute_index_map_;
std::unordered_map<std::string, size_t> GlobalData::g_symbol_index_map_;
std::unordered_map<std::string, bool> GlobalData::g_symbol_trading_end_map_;


StrategyBase::StrategyBase(std::string name,
        std::string config,
        SubscribeMode mode,
        size_t msgs_queue_cap_)
        :strategy_name(std::move(name)),config(std::move(config)), EventSubscriberBase(mode, msgs_queue_cap_)
{
    last_minute_time_index_ = 0;
    strategy_order_id = 0;
    m_traders.clear();

    strategy_orders_vec_.resize(1024*1024);
    trader_order_id_strategy_order_id_map_.resize(1024*1024);
    strategy_order_id_trade_order_id_map_.resize(1024*1024);



    std::fill(strategy_orders_vec_.begin(),strategy_orders_vec_.end(), nullptr);
    std::fill(trader_order_id_strategy_order_id_map_.begin(),trader_order_id_strategy_order_id_map_.end(), -1);
    std::fill(strategy_order_id_trade_order_id_map_.begin(),strategy_order_id_trade_order_id_map_.end(), -1);

    std::string trade_path_ = "./record/"+ GlobalData::g_trading_day_ ;

    boost::filesystem::path trade_path(trade_path_);
    printf("traderecord dir: %s, length %lu\n", trade_path_.c_str(), trade_path_.length());
    try
    {
        if (!boost::filesystem::exists(trade_path))
        {
            boost::filesystem::create_directories(trade_path);
        }
    }
    catch(const boost::filesystem::filesystem_error& e)
    {
        if(e.code() == boost::system::errc::permission_denied)
            printf("%s permission is denied\n", trade_path.c_str());
        else
            printf("create %s error: %s\n", trade_path.c_str(), e.code().message().c_str());
    }
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    strategy_uuid = boost::uuids::to_string(a_uuid);
    auto record_filename = trade_path_ + "/" + strategy_name +  "." + strategy_uuid + ".csv";
    printf("record file name %s\n",record_filename.c_str());
    trade_record.open(record_filename, ios::out|ios::trunc);
    trade_record<<"OrderID,TradeID,OrderRef,Symbol,Direction,OffsetFlag,TradePrice,TradeVolume,Multiple,AssetChange,Commi,"
                  "TradeTime,StrategyName"<<std::endl;

    total_pnl_money = 0;
    total_commi = 0;
}

StrategyBase::~StrategyBase()
{
    if (trade_record.is_open())
        trade_record.close();
    printf("~StrategyBase dtor\n");
}

std::shared_ptr<StrategyBase> StrategyBase::GetSharedPtr(){
    return shared_from_this();
}

bool StrategyBase::Init(){return true;};

void StrategyBase::SetParas(const StrategyParas & paras)
{
    strategyParas  = paras;
    for (auto i = 0; i< SUPPORT_PARA_COUNT; ++i)                   // 第i个参数项
    {
        auto para_len = strategyParas.kpa[i].length_;             // 第i个参数项的参数个数
        if ( para_len == 0)
            break;

        printf("set para, name %s, length %d : \n", strategyParas.kpa[i].name_, para_len);
        for (auto m = 0; m < para_len; m++)      // 第i个参数项的第m个参数值
        {
            printf("%f\t", strategyParas.kpd[i][m]);
        }
        printf("\n");
    }
}

void StrategyBase::RegisterTimer(int delay)
{
    std::function<void(void)> taskRunFunc = [this] { OnTimer(); };
    timer.start(delay, taskRunFunc);
}

bool StrategyBase::AddTrader(TraderType type, const std::string& ref, const std::shared_ptr<TraderBase>& trader)
{
    auto it = m_traders.find(type);
    if  (it == m_traders.end())
    {
        m_traders[type] = SingleTrader{{ref, trader}};

        return true;
    }
    else
    {
        auto it2 = it->second.find(ref);
        if (it2 == it->second.end())
        {
            it->second[ref] = trader;

            return true;
        }
        else
        {
             LOG_ERROR(LOGGER, "trader %s already exist {}", ref.c_str());
            return false;
        }
    }
}

bool StrategyBase::AddEventPublisher(MessageType type, const std::string& ref, const std::shared_ptr<EventPublisherBase>& event_publisher)
{
    auto it = m_publishers.find(type);
    if  (it == m_publishers.end())
    {
        m_publishers[type] = SingleEventPublisher {{ref, event_publisher}};
        if (this->GetSharedPtr() == nullptr)
            printf("strategy null ptr\n");
        event_publisher->RegisterSubscriber(this->GetSharedPtr());
        return true;
    }
    else
    {
        auto it2 = it->second.find(ref);
        if (it2 == it->second.end())
        {
            it->second[ref] = event_publisher;
            event_publisher->RegisterSubscriber(this->GetSharedPtr());
            return true;
        }
        else
        {
             LOG_ERROR(LOGGER, "publisher %s already exist {}", ref.c_str());
            return false;
        }
    }
}

void StrategyBase::Create()
{
    //std::thread();
}



void StrategyBase::OnPushMessage(std::shared_ptr<MessageBase> msg) {
//    printf("%s OnNewMessage\n", strategy_name.c_str());
//    printf("msg id: %d\n", msg->get_msg_id());
//    printf("msg type: %d\n", msg->get_msg_type());
//    printf("msg remark: %s\n", msg->get_msg_remark().c_str());

    auto msg_type = msg->get_msg_type();

    switch (msg_type)
    {
        case MessageType::CTPTICKDATA:
        {
            const auto ctp_tick_data  = std::make_shared<TICK>(msg->get_msg_body().tick);
//            printf("%s on push message %s %s: tick_time: %d, min: %d, last_price: %.2lf, open_price: %.2lf, high_price: %.2lf, low_price: %.2lf, close_price: %.2lf, "
//                   "volume: %d, open_interest: %.2lf, average_price: %.2lf\n", strategy_name.c_str(),
//                   ctp_tick_data->update_time,ctp_tick_data->symbol,
//                   ctp_tick_data->tick_time, Adj2Min(ctp_tick_data->tick_time),
//                   ctp_tick_data->last_price, ctp_tick_data->open_price,
//                   ctp_tick_data->highest_price, ctp_tick_data->lowest_price,
//                   ctp_tick_data->close_price, ctp_tick_data->volume, ctp_tick_data->open_interest, ctp_tick_data->average_price);
//            tradebee_logger->info("{} on push {} {} {}", strategy_name.c_str(), ctp_tick_data->update_time, ctp_tick_data->symbol, ctp_tick_data->last_price);

            symbols_last_ticks[ctp_tick_data->symbol] = ctp_tick_data;
            OnTick(ctp_tick_data);

            try {
                auto now_minute_time_ = Adj2Min(ctp_tick_data->tick_time);
                auto now_minute_time_index_ =  GlobalData::GetMinuteIndex(now_minute_time_);
                if (now_minute_time_index_ > last_minute_time_index_ )
                {
                    OnNewMinute(now_minute_time_);
                    last_minute_time_index_ = now_minute_time_index_;
                }
            }
            catch (...)
            {
               // printf("get %lu minute index fail\n", ctp_tick_data->tick_time);
            }
            break;
        }
        case MessageType::CTPKBARDATA:
        {
            const auto bar = std::make_shared<BAR>(msg->get_msg_body().bar);
            OnBar(bar);

            break;
        }
        case MessageType::RTNORDER:
        {
            const auto & order = std::make_shared<OrderField>(msg->get_msg_body().order);

            //UpdateStrategyOrder(order);
            if (order->OffsetFlag!=OrderOffsetFlag::OPEN && order->Status==OrderStatus::CANCELED)
                UnFrozenPosition(order);
            OnRtnOrder(order);
            break;
        }
        case MessageType::RTNTRAED:
        {
            const auto & trade = std::make_shared<TradeField>(msg->get_msg_body().trade);
            //const auto & order = GetStrategyOrderByTraderOrderID(trade->OrderID);
            //UpdateStrategyOrder(order);
            UpdatePosition(trade);
            OnRtnTrade(trade);

            int direction = trade->Direction == OrderDirection::BUY  ? 1 : -1;
            int offset_flag = trade->OffsetFlag == OrderOffsetFlag::OPEN ? 1 : -1;
            int multiple = GlobalData::GetSymbolField(trade->Symbol).Multiple;
            double assert = trade->TradePrice * trade->TradeVolume * direction * multiple;

            double commi=0.0;

            if (trade->OffsetFlag == OrderOffsetFlag::OPEN )
            {
                commi = trade->TradeVolume * CalSymbolTradeCost(trade->Symbol, trade->TradePrice)->OpenCostPer;
            }
            else if(trade->OffsetFlag == OrderOffsetFlag::CLOSE)             // 平今
            {
                commi = trade->TradeVolume * CalSymbolTradeCost(trade->Symbol, trade->TradePrice)->CloseTodayCostPer;
            }
            else
            {
                commi = trade->TradeVolume * CalSymbolTradeCost(trade->Symbol, trade->TradePrice)->CloseCostPer;
            }

            char record[126]{};
            sprintf(record,"%d,%s,%s,%s,%d,%d,%.3lf,%d,%d,%.2lf,%.2lf,%s",
                    trade->OrderID,
                    trade->TradeID,
                    trade->OrderRef,
                    trade->Symbol,
                    direction,
                    offset_flag,
                    trade->TradePrice,
                    trade->TradeVolume,
                    multiple,
                    -assert,
                    -commi,
                    trade->TradeTime);
            trade_record<<record<<std::endl;
            total_pnl_money += (-assert);
            total_commi += (-commi);
            break;
        }
        case MessageType::STOCKKBARDATA:
            break;
        default:
            printf("msg %d not support", msg_type);
            break;

    }
}

//void StrategyBase::OnNoticeMessage(MessageType type) {
//    printf("%s OnNoticeMessage\n", strategy_name.c_str());
//    printf("msg type: %d\n", type);
//    if (type == MessageType::CTPTICKDATA)
//    {
//
//    }
//}

int StrategyBase::Buy(std::string symbol, double price, int volume)
{
    auto order =  default_ctp_trader->Buy(std::move(symbol), price, volume, GetSharedPtr());
    if(order)
    {
        return AppendStrategyOrder(order);
    }
    return -1;
}

int StrategyBase::Sell(std::string symbol, double price, int volume)
{
    auto order =  default_ctp_trader->Sell(std::move(symbol), price, volume, GetSharedPtr());
    if(order)
    {
        FrozenPosition(order);
        return AppendStrategyOrder(order);

    }
    return -1;
}

int StrategyBase::Short(std::string symbol, double price, int volume)
{
    auto order =  default_ctp_trader->Short(std::move(symbol), price, volume, GetSharedPtr());
    if(order)
    {
        return AppendStrategyOrder(order);
    }
    return -1;
}

int StrategyBase::Cover(std::string symbol, double price,int volume)
{
    auto order =  default_ctp_trader->Cover(std::move(symbol), price, volume, GetSharedPtr());
    if(order)
    {
        FrozenPosition(order);
        return AppendStrategyOrder(order);

    }
    return -1;
}

bool StrategyBase::Cancel(int strategy_order_id_)
{
    return default_ctp_trader->Cancel(GetStrategyOrderByStrategyID(strategy_order_id_), GetSharedPtr());
}

bool StrategyBase::CancelPendingOpenOrder() {
    std::lock_guard<std::mutex> l(strategy_orders_map_mtx_);
    bool allSuccess = true;
    for (const auto& order : strategy_orders_vec_) {
        if ( order && (order->Status != OrderStatus::ALL_TRADED && order->Status != OrderStatus::CANCELED)
              &&(order->OffsetFlag == OrderOffsetFlag::OPEN))
        {
            if(!default_ctp_trader->Cancel(order, GetSharedPtr()))
            {
                allSuccess = false;
                printf("cancel open fail: ");
                order->print();
            }
        }

    }
    return allSuccess;
}

bool StrategyBase::CancelPendingOrder() {
    std::lock_guard<std::mutex> l(strategy_orders_map_mtx_);
    bool allSuccess = true;
    printf("**********  Cancel Pending Order Start  *********\n");
    for (const auto& order : strategy_orders_vec_) {

        if (order && order->Status != OrderStatus::ALL_TRADED && order->Status != OrderStatus::CANCELED)
        {
            if(!default_ctp_trader->Cancel(order, GetSharedPtr()))
            {
                allSuccess = false;
                printf("cancel fail: ");
                order->print();
            }
        }
        //order->print();

    }
    printf("**********   Cancel Pending Order End   *********\n");
    return allSuccess;
}

void StrategyBase::ShowPendingOrder() {
    printf("********************  Pending Order Start  *******************\n");
    for (const auto& order : strategy_orders_vec_) {

        if (order && order->Status != OrderStatus::ALL_TRADED && order->Status != OrderStatus::CANCELED)
        {
            order->print();
        }
    }
    printf("********************   Pending Order End   *******************\n");


}

int StrategyBase::IncreaseStrategyOrderID()
{
    return ++strategy_order_id;
}

int StrategyBase::AppendStrategyOrder(std::shared_ptr<OrderField>& order)
{
    std::lock_guard<std::mutex> l(strategy_orders_map_mtx_);
    auto strategy_order_id_ = IncreaseStrategyOrderID();
    strategy_orders_vec_[strategy_order_id_] = order;

    UpdateOrderIDMap(order->OrderID, strategy_order_id_);
    return strategy_order_id_;

}

void StrategyBase::UpdateStrategyOrder(const shared_ptr<OrderField> &order) {
    auto strategy_order_id_ = GetStrategyOrderID(order->OrderID);
    strategy_orders_vec_[strategy_order_id_] = order;

}


std::shared_ptr<OrderField> StrategyBase::GetStrategyOrderByStrategyID(int strategy_order_id_)
{

    return strategy_orders_vec_[strategy_order_id_];
}

std::shared_ptr<OrderField> StrategyBase::GetStrategyOrderByTraderOrderID(int trader_order_id)
{
    auto strategy_order_id_ = GetStrategyOrderID(trader_order_id);

    return GetStrategyOrderByStrategyID(strategy_order_id_);
}


void StrategyBase::UpdateOrderIDMap(int trader_order_id, int strategy_order_id_)
{
    trader_order_id_strategy_order_id_map_[trader_order_id] = strategy_order_id_;
    strategy_order_id_trade_order_id_map_[strategy_order_id_] = trader_order_id;
}



int StrategyBase::GetStrategyOrderID(int trader_order_id)
{
    return trader_order_id_strategy_order_id_map_[trader_order_id];

}

int StrategyBase::GetTraderOrderID(int strategy_order_id_)
{
    return strategy_order_id_trade_order_id_map_[strategy_order_id_];
}


bool StrategyBase::CreateRelatedOrder() {
    return false;
}

void StrategyBase::RecordTrade(const std::shared_ptr<TradeField> &trade) {

}


SymbolField GlobalData::GetSymbolField(const std::string& symbol)
{
    try
    {
        return g_symbol_map_.at(symbol);
    }
    catch(...)
    {
        printf("get %s symbol info err\n", symbol.c_str());
        return SymbolField{};
    }
}

 TradingPeriod GlobalData::GetProductTradingPeriods(const std::string& product_id)
{
    try {
        return g_product_trading_periods_.at(product_id);
    }
    catch (...)
    {
        printf("product %s trading periods not find\n", product_id.c_str());
        return TradingPeriod{};
    }
}

 void GlobalData::InitSymbolIndex()
{
    size_t i = 0;
    for (auto s  : g_symbol_map_) {
        g_symbol_index_map_[s.first] = i;
        printf("symbol %s index %d\n", s.first.c_str(), i);
        i++;
    }
}

size_t GlobalData::GetSymbolIndex(const std::string& symbol)
{
//        try
//        {
//            return g_symbol_index_map_.at(symbol);
//        }
//        catch(...)
//        {
//
//            printf("symbol %s index not find\n", symbol.c_str());
//            return size_t(-1);
//        }

    return g_symbol_index_map_.at(symbol);
}

 void GlobalData::InitMinuteIndex()
{
    for (size_t i = 0; i < g_day_miute_vector_.size(); ++i) {
        g_minute_index_map_.emplace(g_day_miute_vector_[i], i);
        printf("minute %lu index %lu\n",g_day_miute_vector_[i], i);
    }
}

 size_t GlobalData::GetMinuteIndex(int minute_time_key)
{
//        try
//        {
//            return g_minute_index_map_.at(minute_time_key);
//        }
//        catch(...)
//        {
//
//            printf("minute %d index not find\n", minute_time_key);
//            return (size_t)-1;
//        }

    return g_minute_index_map_.at(minute_time_key);
}

shared_ptr<SymbolTradeCost> StrategyBase::CalSymbolTradeCost(string symbol, double price) {

    auto multi = GlobalData::g_symbol_map_[symbol].Multiple;

    auto commi = default_ctp_trader->getSymbolCommi(symbol);
    auto margin = default_ctp_trader->getSymbolMargin(symbol);
    if (commi == nullptr)
    {
        printf("get %s commi fail\n",symbol.c_str());
        return nullptr;
    }
    if (margin == nullptr)
    {
        printf("get %s margin fail\n",symbol.c_str());
        return nullptr;
    }
//    printf("commi %x\n",commi.get());
//
//    printf("%s commi: %f %f %f %f %f %f\n", symbol.c_str(), commi->OpenRatioByMoney, commi->OpenRatioByVolume, commi->CloseRatioByMoney, commi->CloseRatioByVolume,
//           commi->CloseTodayRatioByMoney, commi->CloseTodayRatioByVolume);


    auto cost = make_shared<SymbolTradeCost>();
    cost->LongMarginCostPer = price * multi * margin->LongMarginRatioByMoney + margin->LongMarginRatioByVolume;
    cost->ShortMarginCostPer = price * multi * margin->ShortMarginRatioByMoney + margin->ShortMarginRatioByVolume;

    cost->OpenCostPer =price * multi * commi->OpenRatioByMoney + commi->OpenRatioByVolume;
    cost->CloseCostPer = price * multi * commi->CloseRatioByMoney + commi->CloseRatioByVolume;
    cost->CloseTodayCostPer = price * multi * commi->CloseTodayRatioByMoney + commi->CloseTodayRatioByVolume;

    return cost;

}

void StrategyBase::ShowAllPosition() {
    printf("#################### %10s Position Start ####################\n", strategy_name.c_str());
    std::lock_guard<std::recursive_mutex > l(position_mtx_);
    for (const auto& p: position)
    {
        if (p.second->LongTotal>0 || p.second->ShortTotal>0)
            p.second->print();
    }
    printf("####################  %10s Position End  ####################\n", strategy_name.c_str());
}

void StrategyBase::CloseAllPosition() {
    std::lock_guard<std::recursive_mutex > l(position_mtx_);
    for (const auto& p: position)
    {
        auto symbol = p.first;
        auto price_tick = GlobalData::GetSymbolField(symbol).PriceTick;

        auto lpc = p.second->LongTotal;           // 多头
        auto spc = p.second->ShortTotal;          // 空头

        if (lpc > 0)
        {
            auto price =symbols_last_ticks[symbol]->bid_price[0] - 3 * price_tick;
            Sell(symbol, price , lpc);
            printf("close all: sell %s, price %f, volume %d\n", symbol.c_str(), price , lpc);
        }

        if (spc > 0)
        {
            auto price =symbols_last_ticks[symbol]->ask_price[0] + 3 * price_tick;
            Cover(symbol, price, spc);
            printf("close all: cover %s, price %f, volume %d\n",  symbol.c_str(), price, spc);
        }
    }

}


std::shared_ptr<PositionField> StrategyBase::GetSymbolPosition(const std::string& symbol)
{
    std::shared_ptr<PositionField> p = nullptr;

    std::lock_guard<std::recursive_mutex > l(position_mtx_);
    auto it_p = position.find(symbol);

    if (it_p != position.end())
    {
        p = it_p->second;
    }
    return p;
}


void StrategyBase::UpdatePosition(const shared_ptr<TradeField> &trade) {
    std::lock_guard<std::recursive_mutex > l(position_mtx_);
    auto it = position.find(trade->Symbol);
    if (it != position.end())
    {
        if (trade->OffsetFlag == OrderOffsetFlag::OPEN)                  // 开
        {

            if (trade->Direction == OrderDirection::BUY)                 // 买开
            {
                it->second->LongTotal += trade->TradeVolume;
                it->second->LongDay += trade->TradeVolume;
                it->second->LongCloseAbleTotal += trade->TradeVolume;
                it->second->LongCloseAbleDay += trade->TradeVolume;

            }
            else                                                       //  卖开
            {
                it->second->ShortTotal += trade->TradeVolume;
                it->second->ShortDay += trade->TradeVolume;
                it->second->ShortCloseAbleTotal += trade->TradeVolume;
                it->second->ShortCloseAbleDay += trade->TradeVolume;
            }
        }
        else if (trade->OffsetFlag == OrderOffsetFlag::CLOSE)             // 平今
        {
            if (trade->Direction == OrderDirection::BUY)                  // 买今平
            {
                it->second->ShortTotal -= trade->TradeVolume;
                it->second->ShortDay -= trade->TradeVolume;
//                it->second->ShortCloseAbleTotal -= trade->TradeVolume;
//                it->second->ShortCloseAbleDay -= trade->TradeVolume;
            }
            else                                                           // 卖今平
            {
                it->second->LongTotal -= trade->TradeVolume;
                it->second->LongDay -= trade->TradeVolume;
//                it->second->LongCloseAbleTotal -= trade->TradeVolume;
//                it->second->LongCloseAbleDay -= trade->TradeVolume;
            }
        }
        else                                                               // 平昨
        {
            if (trade->Direction == OrderDirection::BUY)                  // 买今昨
            {
                it->second->ShortTotal -= trade->TradeVolume;

//                it->second->ShortCloseAbleTotal -= trade->TradeVolume;

            }
            else                                                           // 卖今昨
            {
                it->second->LongTotal -= trade->TradeVolume;

//                it->second->LongCloseAbleTotal -= trade->TradeVolume;

            }
        }
        it->second->print();
    }
    else
    {
        auto p = make_shared<PositionField>();
        strcpy(p->Symbol, trade->Symbol);

        if (trade->OffsetFlag == OrderOffsetFlag::OPEN)                  // 开
        {

            if (trade->Direction == OrderDirection::BUY)                 // 买开
            {
                p->LongTotal = trade->TradeVolume;
                p->LongDay = trade->TradeVolume;
                p->LongCloseAbleTotal = trade->TradeVolume;
                p->LongCloseAbleDay = trade->TradeVolume;
                p->ShortTotal = 0;
                p->ShortDay = 0;
                p->ShortCloseAbleTotal = 0;
                p->ShortCloseAbleDay = 0;
            }
            else                                                       //  卖开
            {
                p->LongTotal = 0;
                p->LongDay = 0;
                p->LongCloseAbleTotal = 0;
                p->LongCloseAbleDay = 0;
                p->ShortTotal = trade->TradeVolume;
                p->ShortDay = trade->TradeVolume;
                p->ShortCloseAbleTotal = trade->TradeVolume;
                p->ShortCloseAbleDay = trade->TradeVolume;
            }
        }
//        else if (trade->OffsetFlag == OrderOffsetFlag::CLOSE)             // 平
//        {
//            if (trade->Direction == OrderDirection::BUY)                  // 买平
//            {
//                p->LongTotal = trade->TradeVolume;
//                p->LongDay = trade->TradeVolume;
//                p->LongCloseAbleTotal = trade->TradeVolume;
//                p->LongCloseAbleDay = trade->TradeVolume;
//                p->ShortTotal = 0;
//                p->ShortDay = 0;
//                p->ShortCloseAbleTotal = 0;
//                p->ShortCloseAbleDay = 0;
//            }
//            else
//            {
//                p->LongTotal = 0;
//                p->LongDay = 0;
//                p->LongCloseAbleTotal = 0;
//                p->LongCloseAbleDay = 0;
//                p->ShortTotal = 0;
//                p->ShortDay = 0;
//                p->ShortCloseAbleTotal = 0;
//                p->ShortCloseAbleDay = 0;
//            }
//        }
//        else                                                               //
//        {
//            if (trade->Direction == OrderDirection::BUY)                  // 买平
//            {
//                p->LongTotal = trade->TradeVolume;
//                p->LongDay = trade->TradeVolume;
//                p->LongCloseAbleTotal = trade->TradeVolume;
//                p->LongCloseAbleDay = trade->TradeVolume;
//                p->ShortTotal = 0;
//                p->ShortDay = 0;
//                p->ShortCloseAbleTotal = 0;
//                p->ShortCloseAbleDay = 0;
//            }
//            else
//            {
//                p->LongTotal = 0;
//                p->LongDay = 0;
//                p->LongCloseAbleTotal = 0;
//                p->LongCloseAbleDay = 0;
//                p->ShortTotal = 0;
//                p->ShortDay = 0;
//                p->ShortCloseAbleTotal = 0;
//                p->ShortCloseAbleDay = 0;
//            }
//        }
        p->print();
        if (p->LongTotal < 0 || p->ShortTotal<0)
        {
            printf("position error\n");
            exit(-1);
        }

        position.emplace(p->Symbol, p);
    }
}

void StrategyBase::FrozenPosition(const shared_ptr<OrderField> &order) {
    std::lock_guard<std::recursive_mutex > l(position_mtx_);
    auto it = position.find(order->Symbol);
    if (it != position.end())
    {
        if (order->OffsetFlag == OrderOffsetFlag::CLOSE)             // 平今
        {
            if (order->Direction == OrderDirection::BUY)                  // 买今平
            {
                it->second->ShortCloseAbleTotal -= order->Volume;
                it->second->ShortCloseAbleDay -= order->Volume;
            }
            else                                                           // 卖今平
            {
                it->second->LongCloseAbleTotal -= order->Volume;
                it->second->LongCloseAbleDay -= order->Volume;
            }
        }
        else  if (order->OffsetFlag == OrderOffsetFlag::CLOSE_YESTERDAY)            // 平昨
        {
            if (order->Direction == OrderDirection::BUY)                  // 买今昨
            {

                it->second->ShortCloseAbleTotal -= order->Volume;

            }
            else                                                           // 卖今昨
            {
                it->second->LongCloseAbleTotal -= order->Volume;
            }
        }
        it->second->print();
    }
}


void StrategyBase::UnFrozenPosition(const shared_ptr<OrderField> &order) {
    std::lock_guard<std::recursive_mutex > l(position_mtx_);
    auto it = position.find(order->Symbol);
    if (it != position.end())
    {
        if (order->OffsetFlag == OrderOffsetFlag::CLOSE)             // 平今
        {
            if (order->Direction == OrderDirection::BUY)                  // 买今平
            {
                it->second->ShortCloseAbleTotal += order->Volume;
                it->second->ShortCloseAbleDay += order->Volume;
            }
            else                                                           // 卖今平
            {
                it->second->LongCloseAbleTotal += order->Volume;
                it->second->LongCloseAbleDay += order->Volume;
            }
        }
        else  if (order->OffsetFlag == OrderOffsetFlag::CLOSE_YESTERDAY)            // 平昨
        {
            if (order->Direction == OrderDirection::BUY)                  // 买今昨
            {

                it->second->ShortCloseAbleTotal += order->Volume;

            }
            else                                                           // 卖今昨
            {
                it->second->LongCloseAbleTotal += order->Volume;
            }
        }
        it->second->print();
    }
}

void StrategyBase::SetTargetPosition(const std::string &symbol, double price, int target) {

    int net_position =0;
    int cur_long = 0;
    int cur_short = 0;


    auto current_position = GetSymbolPosition(symbol);

    if (current_position)
    {
        net_position = current_position->LongCloseAbleTotal - current_position->ShortCloseAbleTotal;
        cur_long = current_position->LongCloseAbleTotal;
        cur_short = current_position->ShortCloseAbleTotal;
    }


    auto task_position = target - net_position;

    printf("%s set target position %d, price %.3lf, net position %d, task position %d, ",
           symbol.c_str(), target, price, net_position, task_position);

    if (task_position > 0)     // 目标持仓大于净头寸，执行头寸大于0     1.平空   2.平空不足，做多
    {
        int to_cover = 0;
        int to_buy = 0;
        if (task_position <= cur_short)
        {
            to_cover = task_position;
        }
        else
        {
            to_cover = cur_short;
            to_buy = task_position - to_cover;
        }
        printf("cover volume %d, buy volume %d\n", to_cover, to_buy);
        if (to_cover>0)
        {
            auto close_strategy_order_id = Cover(symbol,price,to_cover);
            if (close_strategy_order_id != -1)
            {

            }
        }

        if (to_buy>0)
        {
            Buy(symbol,price,to_buy);
        }
    }
    else if (task_position < 0)      // 目标持仓小于净头寸，执行头寸小于0  1.平多   2.平多不足，做多
    {
        int to_sell = 0;
        int to_short = 0;
        if (-task_position <= cur_long)
        {
            to_sell = -task_position;
        }
        else
        {
            to_sell = cur_long;
            to_short = -task_position - to_sell;
        }
        printf("sell volume %d, short volume %d\n", to_sell, to_short);
        if (to_sell>0)
        {
            auto close_strategy_order_id = Sell(symbol,price,to_sell);
            if (close_strategy_order_id != -1)
            {

            }
        }

        if (to_short>0)
        {
            Short(symbol,price,to_short);
        }
    }
    else
    {
        printf("position right\n");
    }

}

ParaAttrField* StrategyBase::getStrategyParasAttr() {
    return strategyParas.kpa;
}

double StrategyBase::getTotalPnlMoney() const {
    return total_pnl_money;
}

double StrategyBase::getTotalCommi() const {
    return total_commi;
}
