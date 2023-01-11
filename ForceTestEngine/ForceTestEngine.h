//
// Created by haichao.zhang on 2022/4/22.
//

#ifndef TRADEBEE_FORCETESTENGINE_H
#define TRADEBEE_FORCETESTENGINE_H

#include "../StrategyBase/StrategyBase.h"
#include "../CTPMD/CTPMarketDataImpl.h"
#include "../Monitor/MonitorBase.h"
#include "../BackTestEngine/SimpleBackTestEngine.h"
#include "../SohaDataCenter/SohaDataCenterStrategy.h"
#include "../KWeightStrategy/KWeightStrategy.h"

#include "../TraderProxy/TraderProxy.h"
#include "../CTPMD/CTPBackTestMarketDataImpl.h"
#include "ThreadPool/ThreadPool.h"


class ForceTestEngine {

private:
    std::shared_ptr<StrategyBase> strategy;
//    std::vector<StrategyParas> paras_vec_;

    std::vector<ParaAttrField> para_attr_;

    // [参数项 所有候选 参数值]
    std::vector<std::vector<std::vector<double>>> candidate_paras;

//    std::shared_ptr<TraderBase> ctp_trader_;
//    std::shared_ptr<CTPMarketDataBase> ctp_tick_publisher_;
//    std::shared_ptr<EventPublisherBase> bar_publisher_;
//    std::shared_ptr<MonitorBase> monitor_;
//    std::shared_ptr<SimpleBackTestEngine> back_test_engine_;
//
//    std::shared_ptr<SohaDataCenterStrategy> dataCenterStrategy_;
//
//    std::shared_ptr<KWeightStrategy> kw_strategy_;

   ThreadPool* threadPool;

   std::atomic<int> task_seqno_{};

   size_t task_num_;


public:

    ForceTestEngine(size_t task_num);


    static std::vector<std::vector<double>> GenIntParas(ParaAttrField& pa);

    bool Init();

    void Start();

    void GenStrategyParasVec();

    void SetParaAttr(ParaAttrField kpa_[], int para_counts_);


    double DoTask(StrategyParas sp);
};


#endif //TRADEBEE_FORCETESTENGINE_H
