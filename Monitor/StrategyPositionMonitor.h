//
// Created by haichao.zhang on 2022/4/13.
//

#ifndef TRADEBEE_STRATEGYPOSITIONMONITOR_H
#define TRADEBEE_STRATEGYPOSITIONMONITOR_H

#include "MonitorBase.h"

class StrategyPositionMonitor : public MonitorBase {
public:

    explicit StrategyPositionMonitor(StopType type):MonitorBase(type)
    {

    }

    bool Init() override;

    void Start() override;

    void OnTick(const std::shared_ptr<TICK> & tick) override;
};


#endif //TRADEBEE_STRATEGYPOSITIONMONITOR_H
