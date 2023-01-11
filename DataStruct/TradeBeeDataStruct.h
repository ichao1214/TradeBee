//
// Created by zhanghaichao on 2021/12/17.
//
#include "string"
#include "cstring"
#include <map>
#include <vector>
#include <memory>
#include "pubilc.h"
#include <unordered_map>

#ifndef TRADEBEE_COMMDATASTRUCT_H
#define TRADEBEE_COMMDATASTRUCT_H

#define MAX_PARA_LENGTH 1024
#define SUPPORT_PARA_COUNT 10   // 支持的参数个数

class StrategyBase;

typedef char TimeType[13];


struct EnumClassHash
{
    template <typename T>
    std::size_t operator()(T t) const
    {
        return static_cast<std::size_t>(t);
    }
};

enum OrderDirection
{
    BUY = 0x00,
    SELL = 0x01
};

enum OrderOffsetFlag
{
    OPEN = 0x00,
    CLOSE = 0x01,
    CLOSE_YESTERDAY = 0x02
};


enum ExchangeID
{
    SHFE = 0x00,
    CZCE = 0x01,
    DCE  = 0x02,
    CFFEX = 0x03,
    INE = 0x04,
    SGE = 0x05,
    SSE = 0x06,
    SZSE = 0x07,
    BSE = 0x08
};


enum class OrderStatus
{
    INSERTED = 0x00,
    ACCEPTED = 0x01,
    REJECTED = 0x02,
    CANCELED = 0x03,
    ALL_TRADED = 0x04,
    PART_TRADED = 0x05,
    INSERTING = 0x06,
    CANCELING = 0x07,
    UNKNOWN = 0x10
};

static std::unordered_map<OrderStatus, std::string, EnumClassHash> OrderStatusMap{{OrderStatus::INSERTED,"INSERTED"},
                                                                   {OrderStatus::ACCEPTED,"ACCEPTED"},
                                                                   {OrderStatus::REJECTED,"REJECTED"},
                                                                   {OrderStatus::CANCELED,"CANCELED"},
                                                                   {OrderStatus::ALL_TRADED,"ALL_TRADED"},
                                                                   {OrderStatus::PART_TRADED,"PART_TRADED"},
                                                                   {OrderStatus::INSERTING,"INSERTING"},
                                                                   {OrderStatus::CANCELING,"CANCELING"},
                                                                   {OrderStatus::UNKNOWN,"UNKNOWN"}};

struct SymbolField
{
    char Symbol[32];
    ExchangeID ExchangeId;
    double PriceTick;
    int Multiple;
    char ProductID[16];
    ///市价单最大下单量
    int	MaxMarketOrderVolume;
    ///市价单最小下单量
    int	MinMarketOrderVolume;
    ///限价单最大下单量
    int	MaxLimitOrderVolume;
    ///限价单最小下单量
    int	MinLimitOrderVolume;

};

struct MarginField
{
    ///多头保证金率
    double	LongMarginRatioByMoney;
    ///多头保证金费
    double	LongMarginRatioByVolume;
    ///空头保证金率
    double	ShortMarginRatioByMoney;
    ///空头保证金费
    double	ShortMarginRatioByVolume;
};

struct CommiField
{
    ///开仓手续费率
    double	OpenRatioByMoney;
    ///开仓手续费
    double	OpenRatioByVolume;
    ///平仓手续费率
    double	CloseRatioByMoney;
    ///平仓手续费
    double	CloseRatioByVolume;
    ///平今手续费率
    double	CloseTodayRatioByMoney;
    ///平今手续费
    double	CloseTodayRatioByVolume;
};

struct SymbolTradeCost
{
    double LongMarginCostPer;
    double ShortMarginCostPer;
    double OpenCostPer;
    double CloseCostPer;
    double CloseTodayCostPer;

};

struct OrderField
{
    int OrderID{};
    char Symbol[32]{};
    OrderDirection Direction;
    OrderOffsetFlag OffsetFlag;
    int Volume{};
    double Price{};
    char OrderRef[64]{};
    int VolumeTraded{};
    TimeType InsertTime{};
    TimeType AcceptedTime{};
    TimeType UpdateTime{};
    OrderStatus Status;
    std::weak_ptr<StrategyBase> strategy;

//    ~OrderField()
//    {
//        if (OrderID!=0)
//            printf("~OrderFeild: %d\n", OrderID);
//    }

    void print()
    {
        printf("order info: symbol %s, order id %d, direction %s, offset %s, volume %d, price %.3lf, order ref %s, volume trade %d, "
               "insert time %s, accept time %s, update time %s, status %s, strategy %x\n",
                  Symbol,
                  OrderID,
                  Direction == OrderDirection::BUY ? "Buy" : "Sell",
                  (OffsetFlag == OrderOffsetFlag::OPEN ) ? "Open" : OffsetFlag == OrderOffsetFlag::CLOSE ? "Close" : "CloseYesterday",
                  Volume,
                  Price,
                  OrderRef,
                  VolumeTraded,
                  InsertTime,
                  InsertTime,
                  AcceptedTime,
                  OrderStatusMap[Status].c_str(),
                  strategy.lock().get());
    }
};

struct TradeField
{
    int OrderID;
    char Symbol[32];
    OrderDirection Direction;
    OrderOffsetFlag OffsetFlag;
    int TradeVolume;
    double TradePrice;
    char TradeID[64];
    char OrderRef[64];
    TimeType TradeTime;


    void print()
    {
        printf("trade info: symbol %s, order id %d, trade id %s, direction %s, offset %s, trade volume %d, trade price %.3lf, order ref %s, "
               "trade time %s\n",
               Symbol,
               OrderID,
               TradeID,
               Direction == OrderDirection::BUY ? "Buy" : "Sell",
               (OffsetFlag == OrderOffsetFlag::OPEN ) ? "Open" : OffsetFlag == OrderOffsetFlag::CLOSE ? "Close" : "CloseYesterday",
               TradeVolume,
               TradePrice,
               OrderRef,
               TradeTime);
    }
};

struct PositionField
{
    char Symbol[32];
    int  LongTotal;             // 多头总持仓
    int  LongDay;               // 多头今持仓
    int  LongCloseAbleTotal;    // 多头总可平
    int  LongCloseAbleDay;      // 多头可平今
    int  ShortTotal;            // 空头总持仓
    int  ShortDay;              // 空头今持仓
    int  ShortCloseAbleTotal;   // 空头总可平
    int  ShortCloseAbleDay;     // 空头可平今

    void print()
    {
        printf("%s long total %d, long today %d, long closeable total %d, long closeable today %d "
              "short total %d, short today %d, short closeable total %d, short closeable today %d\n",
              Symbol, LongTotal, LongDay, LongCloseAbleTotal, LongCloseAbleDay,
                      ShortTotal, ShortDay, ShortCloseAbleTotal, ShortCloseAbleDay);
    }
};


struct TICK
{
    char symbol[32];
    char exchange_id[9];
    char trading_day[9];
    char update_time[13];
    double last_price;
    double pre_settle_price;
    double pre_close_price;
    double pre_open_interest;
    double open_price;
    double highest_price;
    double lowest_price;
    double close_price;
    int volume;
    double average_price;
    double open_interest;
    double amount;
    int tick_time;
    double upper_limit_price;
    double lower_limit_price;
    double bid_price[10];
    int    bid_volume[10];
    double ask_price[10];
    int ask_volume[10];


    void print()
    {
        printf("update time %s, symbol %s, exchange id %s, "
               "tick_time %d, min %d, "
               "last_price %.3lf, "
               "open_price %.3lf, high_price %.3lf, low_price %.3lf, close_price %.3lf, "
               "volume: %d, open_interest: %f, average_price: %f\n",
               update_time, symbol, exchange_id,
               tick_time, Adj2Min(tick_time),
               last_price,
               open_price, highest_price, lowest_price, close_price,
               volume, open_interest, average_price);

        printf("bid price:\t%-8.3lf\t%-8.3lf\t%-8.3lf\t%-8.3lf\t%-8.3lf\t"
                      "ask price:\t%-8.3lf\t%-8.3lf\t%-8.3lf\t%-8.3lf\t%-8.3lf\n",
               bid_price[4], bid_price[3], bid_price[2], bid_price[1],bid_price[0],
               ask_price[0], ask_price[1], ask_price[2], ask_price[3],ask_price[4]);

        printf("bid volume:\t%-11d\t%-11d\t%-11d\t%-11d\t%-11d\t"
                      "ask volume:\t%-11d\t%-11d\t%-11d\t%-11d\t%-11d\n",
               bid_volume[4], bid_volume[3], bid_volume[2], bid_volume[1],bid_volume[0],
               ask_volume[0], ask_volume[1], ask_volume[2], ask_volume[3],ask_volume[4]);
    }

};

struct BAR
{
    char symbol[32];
    size_t bar_time;
    size_t bar_trading_day;
    double open;
    double high;
    double low;
    double close;
    int volume;
    int start_volume;
    double amount;
    double last_amount;
    double start_amount;
    double start_open_interest;
    double open_interest;
    double change_open_interest;
    double price_change;
    double price_change_pct;
    double price_amplitude;
    double pre_close;
    double average;
    int last_volume;
    double average_price_daily;
    int period;

    void print()
    {
        printf("symbol %s, trading day %zu, bar time %zu, open %.3lf, high %.3lf, low %.3lf, close %.3lf, "
               "volume %d, amount %f, average %f, price change %f, price change pct %f, price amplitude %f, "
               "change open interest %f, last volume %d, last amount %f, day average %f, "
               "open interest %f, period %d\n",
               symbol, bar_trading_day, bar_time, open, high, low, close,
               volume, amount, average, price_change, price_change_pct, price_amplitude,
               change_open_interest, last_volume, last_amount, average_price_daily,
               open_interest, period);
    }
};



struct ParaAttrField
{
    double max_value_;
    double min_value_;
    int precision_;
    int length_;
    char name_[32];

};


struct StrategyParas
{
    ParaAttrField kpa[SUPPORT_PARA_COUNT];
    double * kpd[SUPPORT_PARA_COUNT];
};

enum class MessageType
{
    CTPTICKDATA = 0x01,
    CTPKBARDATA = 0x02,
    RTNORDER = 0x30,
    RTNTRAED = 0x40,
    STOCKKBARDATA =0xff
};

enum class BarIntervalType
{
    YEAR  = 0x01,
    QUARTER = 0x02,
    MONTH = 0x03,
    WEEK = 0x04,
    DAY  = 0x05,
    HOUR = 0x06,
    MINUTE = 0x07,
    SECOND = 0x08
};



struct MSG_BODY
{
    TICK tick;
    BAR bar;
    OrderField order;
    TradeField trade;

};

struct TradingPeriod
{
    std::string type;
    int period_counts;
    std::vector<std::pair<int, std::pair<int, int>>> periods;
};

struct StopOrderPara
{
    double stop_price_tick;
    double trail_price_tick;

    StopOrderPara(double s , double t):stop_price_tick(s), trail_price_tick(t)
    {

    }
};

enum class TraderType
{
    CTP = 0x01,
    ZMQ = 0x02,
    BACKTEST =0xff
};

enum class StopType
{
    STP = 0x01,            // 对价止损
    LIMIT = 0x02,          // 限价止损
    TRAIL =0x03            // 移动止损
};



typedef std::map<std::string, TradingPeriod> ProductTradingPeriod ;

typedef std::vector<std::shared_ptr<BAR>> SYMBOLMINUTEBARS;

typedef std::vector<SYMBOLMINUTEBARS> DAYSYMBOLMINUTEBARS;


#endif //TRADEBEE_COMMDATASTRUCT_H
