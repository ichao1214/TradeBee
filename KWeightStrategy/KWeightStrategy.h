//
// Created by haichao.zhang on 2022/4/19.
//

#ifndef TRADEBEE_KWEIGHTSTRATEGY_H
#define TRADEBEE_KWEIGHTSTRATEGY_H

#include <utility>

#include "../M1Strategy/M1Strategy.h"



class KWeightStrategy: public M1Strategy {

public:

    explicit KWeightStrategy(const std::shared_ptr<SohaDataCenterStrategy> &soha = nullptr,
                        const std::shared_ptr<MonitorBase>&  monitor = nullptr,
                        SubscribeMode mode = SubscribeMode::PUSH,
                        size_t msgs_queue_cap_ = 65535, std::string name = "kw", std::string config = "m1.xml",
                        int need_count_ = 10,
                        size_t top_N_ = 3
                        )
                        : need_bar_count(need_count_), topN(top_N_),
                          M1Strategy(soha, monitor, mode, msgs_queue_cap_,
                                     std::move(name), std::move(config))
    {

        position.clear();
        minute_index_bar_ready_map_.clear();
        printf("KWeightStrategy constructor: soha %x, monitor %x,mode %d, msg queue cap %lu, strategy name %s, config %s\n",
               soha.get(), monitor.get(), mode, msgs_queue_cap_, strategy_name.c_str(), StrategyBase::config.c_str());


    }

    ~KWeightStrategy() override
    {
        printf("~KWeightStrategy dtor\n");
    }

    bool Init() override;

    void OnAction(size_t & pre_minute_time_key_index) override;

    void OnRtnOrder(const std::shared_ptr<OrderField> & order) override;

    void OnRtnTrade(const std::shared_ptr<TradeField> &trade) override;


private:
    // strategy paras
    int need_bar_count;     // 往前取n根bar
    size_t topN;        // 取前几个标的（多空各前N）

};


#endif //TRADEBEE_KWEIGHTSTRATEGY_H
