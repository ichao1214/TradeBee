//
// Created by haichao.zhang on 2022/4/22.
//

#include <cmath>
#include <numeric>

#include <algorithm>
#include "logger/LogManager.h"
#include "ForceTestEngine.h"



#define FORCE_TEST_LOGGER LogManager::GetLogger("forcetest", "forcetest.log")

ForceTestEngine::ForceTestEngine(size_t task_num): task_num_(task_num)
{
    task_seqno_ = 0;
    threadPool = new ThreadPool(task_num_);

}



std::vector<std::vector<double>> ForceTestEngine::GenIntParas(ParaAttrField &pa) {
    int start = int(pa.min_value_ * pow(10, double (pa.precision_)));
    int end = int(pa.max_value_ * pow(10, double (pa.precision_)));
    auto total_line = uint64_t ( pow( double (end - start + 1), double (pa.length_)));
    printf("ForceTestEngine: para %s, min value %f (%d), max value %f (%d), precision %d, length %d, total line %lu\n",
           pa.name_, pa.min_value_, start ,pa.max_value_, end, pa.precision_, pa.length_, total_line);

    DecomposeInteger d(end, pa.length_);     // 把end 拆分成 length个数的和
    d.Decompose();
    auto & ret = d.result;
    printf("para candidate size: %zu\n",ret.size());
    std::vector<std::vector<double>> para;
    for (const auto& k : ret)
    {
        std::vector<double> p(k.size());
        //print_container(k);

        //printf("\n");

        std::transform(k.begin(),k.end(),
                       p.begin(), [](int r){return (double) r/100 ;} );
        para.push_back(p);

    }

    return para;

}


void ForceTestEngine::GenStrategyParasVec() {

    for (auto i = 0; i < para_attr_.size(); ++i)            // 第i个参数
    {
        if(para_attr_[i].length_>0)                         // 第i个参数项目参数值的个数
        {
            auto para = GenIntParas(para_attr_[i]);      // 第i个参数所有候选参数值
            candidate_paras.push_back(para);
        }
        else
        {
            break;
        }
    }

}

void ForceTestEngine::SetParaAttr(ParaAttrField kpa_[], int para_counts_) {

    for (int i = 0; i < para_counts_; ++i)
    {
        para_attr_.push_back(kpa_[i]);
    }
}

bool ForceTestEngine::Init() {
    auto paraAttrField = new ParaAttrField[SUPPORT_PARA_COUNT];

    // memset(paraAttrField, 0 , sizeof (paraAttrField) * SUPPORT_PARA_COUNT);
    // k0 涨跌幅权重
    paraAttrField[0].length_=10;
    paraAttrField[0].max_value_=1.00;
    paraAttrField[0].min_value_=0.0;
    paraAttrField[0].precision_=2;
    strcpy(paraAttrField[0].name_, "k0");


    SetParaAttr(paraAttrField, SUPPORT_PARA_COUNT);
    GenStrategyParasVec();
    return true;
}

void ForceTestEngine::Start() {
    StrategyParas cp{};
    std::vector< std::future<double> > results;

    CTPBackTestTraderApiImpl::InitFromBinFile();

    for (auto i = 0; i <  SUPPORT_PARA_COUNT; ++i)             // 第i个参数项
    {
        cp.kpa[i] = para_attr_[i];
        auto para_len = cp.kpa[i].length_;           // 第i个参数项的 的参数项个数
        if (para_len == 0)                            // 第一个参数个数为0的参数，代表参数截至，后面不在有参数
            break;
        for(auto j = 0 ; j <  candidate_paras[i].size(); ++j)      // 第i个参数项的 第j个候选项
        {
//            printf("get test para: ");
//            print_container(candidate_paras[i][j]);
//            printf("\n");
            // 准备参数
            cp.kpd[i] = new double [para_len];

            for (auto m = 0; m < para_len; ++m)              // 第i个参数项的 第j个候选的 第m个参数值
            {
                cp.kpd[i][m] = candidate_paras[i][j][m];
                //printf("set %d %d: %f\n", i,m, cp->kpd[i][m]);
            }

            auto task = [this, cp] {  return DoTask(cp);  };
            results.emplace_back(threadPool->enqueue(task));

//            auto d = DoTask(cp);
//            printf("pnl %f\n", d);
        }
    }
    printf("tasks count: %zu\n", results.size());
    for(auto && result: results)
    {
        auto pnl = result.get();

        printf( "pnl %f\n", pnl) ;
    }


}

double ForceTestEngine::DoTask(StrategyParas sp)
{
    auto task_count = task_seqno_.fetch_add(1) + 1;
    LOG_INFO(FORCE_TEST_LOGGER,"start task {}", task_count);
    printf("start task %d\n", task_count);
    ///prepare Test begin
    auto back_test_engine = std::make_shared<SimpleBackTestEngine>();

    auto ctp_trader = std::make_shared<TraderProxy>("backtest", back_test_engine);
    if (ctp_trader == nullptr)
    {
        LOG_ERROR(LOGGER,"backtest trader create error");
        throw "backtest trader create error";
    }

    if (!ctp_trader->Init() || !ctp_trader->Login("", ""))
        throw "ctp_trader init or login error";

    LOG_INFO(LOGGER,"use ctp backtest md");

    auto ctp_tick_publisher = std::make_shared<CTPBackTestMarketDataImpl>();

    if (ctp_tick_publisher == nullptr)
    {
        LOG_ERROR(LOGGER,"ctp backtest tick publisher create error");
        throw "ctp backtest tick publisher create error";
    }

    if (!ctp_tick_publisher->Init())
        throw "ctp tick publisher  error";

    ctp_tick_publisher->SubscribeCTPMD(MessageType::CTPTICKDATA, ctp_trader->get_all_symbols());

    if(ctp_tick_publisher
       && ctp_tick_publisher->RegisterSubscriber(back_test_engine))
        LOG_INFO(LOGGER,"add ctp tick publisher to backtest engine success");


    auto dataCenterStrategy = std::make_shared<SohaDataCenterStrategy>();

    if(dataCenterStrategy == nullptr)
    {
        LOG_ERROR(LOGGER,"dataCenterStrategy is null");
    }


    if(ctp_trader
       && dataCenterStrategy->AddTrader(TraderType::CTP, "ctp", ctp_trader))
        LOG_INFO(LOGGER,"add ctp trade to strategy success");

    if(ctp_tick_publisher
       && dataCenterStrategy->AddEventPublisher(MessageType::CTPTICKDATA, "ctp_tick", ctp_tick_publisher))
        LOG_INFO(LOGGER,"add ctp tick publisher to strategy success");



    // strategy position monitor
    auto monitor = std::make_shared<StrategyPositionMonitor>(StopType::TRAIL);
    if(ctp_tick_publisher
       && monitor->AddEventPublisher(MessageType::CTPTICKDATA, "ctp_tick", ctp_tick_publisher))
        LOG_INFO(LOGGER,"add ctp tick publisher to monitor success");

    // kw start
    auto kw_strategy = std::make_shared<KWeightStrategy>(dataCenterStrategy, monitor);

    if(kw_strategy == nullptr)
    {
        LOG_ERROR(LOGGER,"kw_strategy is null");
    }

    if(ctp_trader
       && kw_strategy->AddTrader(TraderType::CTP, "ctp", ctp_trader))
        LOG_INFO(LOGGER,"add ctp trade to strategy success");


    if(ctp_tick_publisher
       && kw_strategy->AddEventPublisher(MessageType::CTPTICKDATA, "ctp_tick", ctp_tick_publisher))
        LOG_INFO(LOGGER,"add ctp tick publisher to strategy success");

    if(dataCenterStrategy
       && kw_strategy->AddEventPublisher(MessageType::CTPKBARDATA, "bar", dataCenterStrategy))
        LOG_INFO(LOGGER,"add bar publisher to strategy success");


    dataCenterStrategy->Init();

    monitor->Init();

    kw_strategy->Init();

    // set para
    for (auto i = 0; i <  SUPPORT_PARA_COUNT; ++i)                // 第i个参数项
    {
        if (sp.kpa[i].length_ == 0)
            break;

        std::string pp{};
        for (auto m = 0; m < sp.kpa[i].length_; ++m)
        {
            printf("set %d %d: %f\n", i, m, sp.kpd[i][m]);     // 第i个参数项的 第m个参数值
            pp += std::to_string(sp.kpd[i][m]) + '\t';
        }
        LOG_INFO(FORCE_TEST_LOGGER,"[{}] set para {}: {}", kw_strategy->strategy_uuid, sp.kpa[i].name_ , pp);
    }

    //auto ssp = std::make_unique<StrategyParas>();



    kw_strategy->SetParas(sp);
    /// prepare test end


    /// run test begin
    dataCenterStrategy->Start();
    monitor->Start();
    kw_strategy->Start();
    back_test_engine->StartMatchTaskThread();
    ctp_tick_publisher->StartPushMarketDataThread();
    /// run test end
    LOG_INFO(FORCE_TEST_LOGGER,"task id {}, {} [{}] started",
                          task_count, kw_strategy->strategy_name, kw_strategy->strategy_uuid);
    printf("task id %d, %s [%s] started\n",
                            task_count, kw_strategy->strategy_name.c_str(), kw_strategy->strategy_uuid.c_str());
    /// clear test begin
    kw_strategy->JoinPullThread();


    ctp_tick_publisher->JoinPushMarketDataThread();
    printf("push market data finish\n");

    back_test_engine->StopTask();
    back_test_engine->JoinMatchTaskThread();
    printf("match task finish\n");
    printf("[%s] total pnl money %f, total commi %f",
           kw_strategy->strategy_uuid.c_str(), kw_strategy->getTotalPnlMoney(), kw_strategy->getTotalCommi());
    LOG_INFO(FORCE_TEST_LOGGER,"[{}] total pnl money {}, total commi {}",
                            kw_strategy->strategy_uuid, kw_strategy->getTotalPnlMoney(), kw_strategy->getTotalCommi());

    LOG_INFO(FORCE_TEST_LOGGER, "finish task {}", task_seqno_.load());
    printf("finish task %d\n", task_count);

    /// clear test end

    auto pnl = kw_strategy->getTotalPnlMoney();

    printf("ctp_tick_publisher use count %ld\n", ctp_tick_publisher.use_count());
    printf("ctp_trader use count %ld\n", ctp_trader.use_count());
    printf("back_test_engine use count %ld\n", back_test_engine.use_count());
    printf("kw_strategy point use count %ld\n", kw_strategy.use_count());
    printf("data center point use count %ld\n", dataCenterStrategy.use_count());
    printf("monitor use count %ld\n", monitor.use_count());

    return pnl;

}




