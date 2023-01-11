//
// Created by zhanghaichao on 2021/12/20.
//

#ifndef TRADEBEE_SOHADATACENTERSTRATEGY_H
#define TRADEBEE_SOHADATACENTERSTRATEGY_H

#include <utility>

#include "../StrategyBase/StrategyBase.h"

#include "../BarSeries/BarSeries.h"


class M1Strategy;

class SohaDataCenterStrategy : public StrategyBase, public EventPublisherBase{
public:
    SohaDataCenterStrategy(SubscribeMode mode = SubscribeMode::PUSH,
                           size_t msgs_queue_cap_ = 65535,
                           std::string name = "soha_datacenter",
                           std::string config = "soha.xml")
                 :StrategyBase(std::move(name), std::move(config), mode, msgs_queue_cap_)
    {
        symbol_trade_costs_map_.clear();
        minute_k_bar_series.clear();
        symbol_last_bar_time_map_.clear();
        printf("SohaDataCenterStrategy constructor: mode %d\n", mode);
    }


    ~SohaDataCenterStrategy()
    {
        bar_center.erase(bar_center.begin(), bar_center.end());
        minute_k_bar_series.erase(minute_k_bar_series.begin(), minute_k_bar_series.end());
        symbol_last_bar_time_map_.erase(symbol_last_bar_time_map_.begin(), symbol_last_bar_time_map_.end());

        printf("~SohaDataCenterStrategy dtor\n");
    }

    void OnTimer() override;

    virtual bool Init() override;

    virtual void Start() override;

    virtual void OnTick(const std::shared_ptr<TICK> &tick) override;

    virtual void OnBar(const std::shared_ptr<BAR> & bar) {};


//    void Subscribe(MessageType type, std::vector<std::string> symbols) override;

    void UpdateBar(const std::shared_ptr<TICK> &tick);

    void UpdateSymbolTradeCost(const std::string& symbol, double price);

    bool SaveMinuteBarToDB(int symbol_bar_seq, const std::shared_ptr<BAR> &bar);

    size_t LoadMinuteBarFromDB(const std::string& start_days, const std::string& end_day, const std::vector<std::string>& symbols);

    std::unordered_map<std::string, std::vector<std::shared_ptr<BAR>>> GetSymbolBar(const std::vector<std::string>& symbols)
    {
        std::unordered_map<std::string, std::vector<std::shared_ptr<BAR>>> symbols_bars;
        for (const auto& symbol : symbols)
        {
            try
            {
                auto bar_series = minute_k_bar_series.at(symbol);
                symbols_bars[symbol] = bar_series->k_bars;
            }
            catch(...)
            {
                printf("get %s bar error", symbol.c_str());
            }
        }
        return symbols_bars;
    }

private:


public:

    friend class M1Strategy;

private:
    int bar_seq;

    //

    std::unordered_map<std::string, std::vector<std::pair<int, std::shared_ptr<BAR>>>> minute_k_bars;



    std::unordered_map<std::string, int> symbol_last_bar_time_map_;
    std::unordered_map<std::string, int> symbol_last_bar_seq_map_;

    std::unordered_map<std::string, std::shared_ptr<BarSeries>> minute_k_bar_series;

    std::mutex trade_cost_mtx_;
    std::unordered_map<std::string, std::shared_ptr<SymbolTradeCost>> symbol_trade_costs_map_;

private:

    std::string save2db;


    int IncreaseSymbolBarSeq(const std::shared_ptr<BAR> &close_bar) ;

public:
    DAYSYMBOLMINUTEBARS bar_center;

public:
    std::shared_ptr<DAYSYMBOLMINUTEBARS> getBarCenter() ;

public:

    void UpdateBarCenter(std::shared_ptr<BAR> &load_bar);

//    std::shared_ptr<BAR> GetBar(size_t minute_index, size_t symbol_index);
//
//    std::shared_ptr<BAR> GetBar(size_t minute_time, const std::string& symbol);

    size_t minute_index_size;

    size_t symbol_index_size;

};



#endif //TRADEBEE_SOHADATACENTERSTRATEGY_H
