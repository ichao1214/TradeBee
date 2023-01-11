//
// Created by haichao.zhang on 2022/3/20.
//

#ifndef TRADEBEE_M1STRATEGY_H
#define TRADEBEE_M1STRATEGY_H

#include <utility>

#include "../StrategyBase/StrategyBase.h"
#include "../SohaDataCenter/SohaDataCenterStrategy.h"
#include "../Monitor/StrategyPositionMonitor.h"

class SohaDataCenterStrategy;

class M1Strategy: public StrategyBase {

public:

    explicit M1Strategy(const std::shared_ptr<SohaDataCenterStrategy> &soha = nullptr,
                        std::shared_ptr<MonitorBase>  monitor = nullptr,
                        SubscribeMode mode = SubscribeMode::PUSH,
                        size_t msgs_queue_cap_ = 65535, std::string name = "m1", std::string config = "m1.xml"
                        )
                        :soha(soha),
                        strategyPositionMonitor(std::move(monitor)),
                        StrategyBase(std::move(name),std::move(config),
                                     mode, msgs_queue_cap_)
    {

        position.clear();
        minute_index_bar_ready_map_.clear();
        printf("M1Strategy constructor: soha %x, monitor %x,mode %d, msg queue cap %lu, strategy name %s, config %s\n",
               soha.get(), monitor.get(), mode, msgs_queue_cap_, strategy_name.c_str(), StrategyBase::config.c_str());
    }

    ~M1Strategy() override
    {
        printf("~M1Strategy dtor\n");
    }



    bool Init() override;

    void Start() override;

    void OnTick(const std::shared_ptr<TICK> &tick) override;

    void OnBar(const std::shared_ptr<BAR> & bar) override;

    void OnNewMinute(int & minute_time_key) override;

    void OnAction(size_t & pre_minute_time_key_index) override;

    void OnRtnOrder(const std::shared_ptr<OrderField> & order) override;

    void OnRtnTrade(const std::shared_ptr<TradeField> &trade) override;

    void OnTimer() override;


protected:
    std::shared_ptr<SohaDataCenterStrategy> soha;


    std::shared_ptr<MonitorBase> strategyPositionMonitor;

    bool CheckPreMinuteStrategyBarReady(size_t & pre_minute_index);

    std::unordered_map<int, bool> minute_index_bar_ready_map_;

    std::vector<std::string> strategy_products;





    bool CheckTradingTime(const size_t &pre_minute_time_key_index);
};



#endif //TRADEBEE_M1STRATEGY_H
