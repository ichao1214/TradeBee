//
// Created by zhanghaichao on 2021/12/20.
//

#ifndef TRADEBEE_STRATEGYBASE_H
#define TRADEBEE_STRATEGYBASE_H

#include <unordered_map>
#include <memory>

#include "../TraderBase/TraderBase.h"
#include "../DataStruct/TradeBeeDataStruct.h"
#include "Timer.h"
#include "../EventBase/EventSubscriberBase.h"
#include "../EventBase/EventPublisherBase.h"
#include "pubilc.h"
#include "mysql_helper/MySqlHelper.h"
#include "../StrategyBase/StrategyBase.h"
#include "logger/LogManager.h"
//#include "../SohaDataCenter/SohaDataCenterStrategy.h"
//#include "../Monitor/StrategyPositionMonitor.h"
//#include "../BarSeries/BarSeries.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <fstream>
#include <atomic>
#include <utility>

//

#define LOGGER LogManager::GetLogger("tradebee", "tradebee.log")

class TraderBase;


class GlobalData
{
public:
    // init global symbol begin
    static std::unordered_map<std::string, SymbolField> g_symbol_map_;
    static std::unordered_map<std::string, size_t> g_symbol_index_map_;
    static std::unordered_map<std::string, bool> g_symbol_trading_end_map_;
    static std::atomic<bool> g_symbol_init_flag_;
    // init global symbol end

    static std::string g_trading_day_;
    static std::atomic<bool> g_trading_day_init_flag_;

    // trading periods begin
    static ProductTradingPeriod g_product_trading_periods_;
    static std::vector<size_t> g_day_miute_vector_;
    static std::unordered_map<int, size_t> g_minute_index_map_;
    // trading periods end

    static SymbolField GetSymbolField(const std::string& symbol);

    static TradingPeriod GetProductTradingPeriods(const std::string& product_id);

    static void InitSymbolIndex();

    static size_t GetSymbolIndex(const std::string& symbol);

    static void InitMinuteIndex();

    static size_t GetMinuteIndex(int minute_time_key);


};

typedef std::unordered_map<std::string, std::shared_ptr<TraderBase>> SingleTrader;

typedef std::unordered_map<TraderType, SingleTrader, EnumClassHash> TraderCollection;

typedef std::unordered_map<std::string, std::shared_ptr<EventPublisherBase>> SingleEventPublisher;

typedef std::unordered_map<MessageType, SingleEventPublisher, EnumClassHash> EventPublisherCollection;

class StrategyBase : public EventSubscriberBase, public std::enable_shared_from_this<StrategyBase>
{
private:
    std::ofstream trade_record;

    double total_pnl_money;
public:
    double getTotalPnlMoney() const;

    double getTotalCommi() const;

    std::string strategy_name;
private:
    double total_commi;

protected:
    TraderCollection m_traders;

    EventPublisherCollection m_publishers;

    Timer timer;

    std::string config;

    std::vector<std::string> strategy_symbols;



    std::shared_ptr<TraderBase> default_ctp_trader;

    std::shared_ptr<EventPublisherBase> default_ctp_tick_publisher;

    std::unordered_map<std::string, std::vector<TICK>> symbols_ticks;

    std::unordered_map<std::string, std::shared_ptr<TICK>> symbols_last_ticks;

    std::unordered_map<std::string, std::vector<std::shared_ptr<BAR>>> symbols_bars;

    std::mutex strategy_orders_map_mtx_;
    std::vector<std::shared_ptr<OrderField>> strategy_orders_vec_;

    int strategy_order_id;

    std::mutex trader_order_id_strategy_order_id_map_mtx_;
    std::vector<int> trader_order_id_strategy_order_id_map_;

    std::mutex strategy_order_id_trade_order_id_map_mtx_;
    std::vector<int> strategy_order_id_trade_order_id_map_;

    std::shared_ptr<sql::Connection> mysql_conn;
    std::shared_ptr<MySqlHelper> mySqlHelper;

    size_t last_minute_time_index_;

    std::atomic<bool> running{};
    std::atomic<bool> trading{};

    std::recursive_mutex  position_mtx_;
    std::unordered_map<std::string, std::shared_ptr<PositionField>> position;

    StrategyParas strategyParas;




public:
    ParaAttrField* getStrategyParasAttr();

    std::string strategy_uuid;

protected:

    void ShowAllPosition();

    void CloseAllPosition();

    std::shared_ptr<PositionField> GetSymbolPosition(const std::string& symbol);

    void UpdatePosition(const std::shared_ptr<TradeField> &trade);

    void FrozenPosition(const std::shared_ptr<OrderField> &order);

    void UnFrozenPosition(const std::shared_ptr<OrderField> &order);

public:
    explicit StrategyBase(std::string name = "default_strategy",
                 std::string config = "strategy.xml",
                 SubscribeMode mode = SubscribeMode::PUSH,
                 size_t msgs_queue_cap_ = 65535);

    std::shared_ptr<StrategyBase> GetSharedPtr();

    virtual ~StrategyBase();

    virtual bool Init();

    virtual void RegisterTimer(int delay);

    int Buy(std::string symbol, double price, int volume);

    int Sell(std::string symbol, double price, int volume);

    int Short(std::string symbol, double price, int volume);

    int Cover(std::string symbol, double price,int volume);

    bool Cancel(int strategy_order_id);

    bool CancelPendingOpenOrder() ;

    bool CancelPendingOrder() ;

    void ShowPendingOrder();

    virtual void OnTimer() {};

    virtual void Start() {};

    virtual void OnTick(const std::shared_ptr<TICK> & tick) {};

    virtual void OnBar(const std::shared_ptr<BAR> & bar){};

    virtual void OnNewMinute(int & minute_time_key){};

    virtual void OnAction(size_t & pre_minute_time_key_index){};

    virtual void OnRtnOrder(const std::shared_ptr<OrderField> & order){};

    virtual void OnRtnTrade(const std::shared_ptr<TradeField> &trade) {};

    void OnPushMessage(std::shared_ptr<MessageBase> msg) override;

//    void OnNoticeMessage(MessageType type) override;


    void Create();

    void SetParas(const StrategyParas & paras);



    bool AddTrader(TraderType type, const std::string& ref, const std::shared_ptr<TraderBase>& trader);
    bool AddEventPublisher(MessageType type, const std::string& ref, const std::shared_ptr<EventPublisherBase>& event_publisher);

    bool CreateRelatedOrder();

    const  std::string & GetStrategyName() const { return strategy_name; }

    int IncreaseStrategyOrderID();

    int AppendStrategyOrder(std::shared_ptr<OrderField>& order);

    void UpdateStrategyOrder(const std::shared_ptr<OrderField>& order);

    std::shared_ptr<OrderField> GetStrategyOrderByStrategyID(int strategy_order_id);

    std::shared_ptr<OrderField> GetStrategyOrderByTraderOrderID(int trader_order_id);


    void UpdateOrderIDMap(int trader_order_id, int strategy_order_id);

    int GetStrategyOrderID(int trader_order_id);

    int GetTraderOrderID(int strategy_order_id);

    void RecordTrade(const std::shared_ptr<TradeField> &trade);

    std::shared_ptr<SymbolTradeCost>  CalSymbolTradeCost(std::string symbol, double price);

    void SetTargetPosition(const std::string &symbol, double price, int target);
};



extern "C"
{
ExchangeID ExchangeIDConvert(const char *exchange);
const char *ExchangeNameConvert(ExchangeID exchangeId);
}

#endif //TRADEBEE_STRATEGYBASE_H
