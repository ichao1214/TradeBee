#include <iostream>
#include <signal.h>
#include "../TraderProxy/TraderProxy.h"
#include<chrono>
#include<thread>
#include "cmdline.h"
#include "logger/LogManager.h"

#include "../CTPMD/CTPMarketDataImpl.h"
#include "../CTPMD/CTPBackTestMarketDataImpl.h"


#include "../SohaDataCenter/SohaDataCenterStrategy.h"
//#include "../M1Strategy/M1Strategy.h"
#include "../KWeightStrategy/KWeightStrategy.h"
#include "../ForceTestEngine/ForceTestEngine.h"



using namespace std;

int LoadTradingPeriod();

int main(int argc, char * argv[])
{
    signal(SIGPIPE, SIG_IGN);

    cmdline::parser a;
    a.add("ctp", '\0', "if use ctp");
    a.add("stock", '\0', "if use stock");
    a.add("backtest", '\0', "if use backtest");
    a.add<int>("forcetest_thread", 't', "force test thread numbers", false, 5);
    a.add("ctp_tick", '\0', "if subscribe ctp_tick");
    a.parse_check(argc, argv);

   // std::shared_ptr<TraderProxy> backtest_trader = nullptr;
//    std::shared_ptr<CTPBackTestMarketDataImpl> backtest_md = nullptr;

    std::shared_ptr<TraderProxy> ctp_trader = nullptr;
    std::shared_ptr<TraderProxy> stock_trader = nullptr;

    std::shared_ptr<CTPMarketDataBase> ctp_tick_publisher = nullptr;

    std::shared_ptr<EventPublisherBase> bar_publisher = nullptr;

    std::shared_ptr<SimpleBackTestEngine> back_test_engine = nullptr ;

    std::shared_ptr<MonitorBase> monitor = nullptr;

    bool real_mode = false;

    LoadTradingPeriod();

    try
    {
        if (a.exist("backtest"))
        {
            real_mode = false;
        }
        else if (a.exist("ctp"))
        {
            real_mode = true;
        }


        if (!real_mode)                    // 回测
        {

            LOG_INFO(LOGGER, "use ctp backtest quote");

            // force test
            auto force_test = std::make_shared<ForceTestEngine>(a.get<int>("forcetest_thread"));

            force_test->Init();

            force_test->Start();


        }
        else                                              // 实盘
        {
             LOG_INFO(LOGGER, "real mode use ctp");
            ctp_trader = std::make_shared<TraderProxy>("ctp");

            if (ctp_trader == nullptr)
            {
                 LOG_ERROR(LOGGER, "ctp trader create error");
                return -1;
            }

            if (!ctp_trader->Init() || !ctp_trader->Login("", ""))
            {
                 LOG_ERROR(LOGGER, "ctp trader init or login failed");
                printf("ctp trader init or login failed, exit -1\n");
                return -1;
            }

             LOG_INFO(LOGGER, "use ctp tick quote");
            ctp_tick_publisher= std::make_shared<CTPMarketDataImpl>();

            if (ctp_tick_publisher == nullptr)
            {
                 LOG_ERROR(LOGGER, "ctp tick publisher create failed");
                printf("ctp tick publisher create failed, exit -1\n");
                return -1;
            }

            if (!ctp_tick_publisher->Init() || !ctp_tick_publisher->Login("", ""))
                return -1;

            ctp_tick_publisher->SubscribeCTPMD(MessageType::CTPTICKDATA, ctp_trader->get_all_symbols());

//          初始化策略相关

            auto dataCenterStrategy = std::make_shared<SohaDataCenterStrategy>();

            if(dataCenterStrategy == nullptr)
            {
                 LOG_ERROR(LOGGER, "dataCenterStrategy is null");


            }

//        if(backtest_trader
//           &&  dataCenterStrategy->AddTrader(TraderType::BACKTEST, "test", backtest_trader))
//             LOG_INFO(LOGGER, "add backtest trade to strategy success");

            if(ctp_trader
               && dataCenterStrategy->AddTrader(TraderType::CTP, "ctp", ctp_trader))
                 LOG_INFO(LOGGER, "add ctp trade to strategy success");

            if(stock_trader
               && dataCenterStrategy->AddTrader(TraderType::BACKTEST, "stock", stock_trader))
                 LOG_INFO(LOGGER, "add stock trade to strategy success");

            if(ctp_tick_publisher
               && dataCenterStrategy->AddEventPublisher(MessageType::CTPTICKDATA, "ctp_tick", ctp_tick_publisher))
                 LOG_INFO(LOGGER, "add ctp tick publisher to strategy success");

            dataCenterStrategy->Init();

            dataCenterStrategy->Start();

            // strategy position monitor
            monitor = std::make_shared<StrategyPositionMonitor>(StopType::TRAIL);
            if(ctp_tick_publisher
               && monitor->AddEventPublisher(MessageType::CTPTICKDATA, "ctp_tick", ctp_tick_publisher))
                 LOG_INFO(LOGGER, "add ctp tick publisher to monitor success");

            monitor->Init();

            monitor->Start();

            bar_publisher = dataCenterStrategy;

            // m1 strategy begin


            /*
            //auto m1_strategy = std::make_shared<M1Strategy>(dataCenterStrategy, monitor,SubscribeMode::PULL, 1 << 21);   // 1 << 21 = 2 ^ 21 = 2097152

            //auto m1_strategy = std::make_shared<KWeightStrategy>(dataCenterStrategy, monitor);
            auto m1_strategy = std::make_shared<M1Strategy>(dataCenterStrategy, monitor);

            if(m1_strategy == nullptr)
            {
                 LOG_ERROR(LOGGER, "m1_strategy is null");
            }

    //        if(backtest_trader
    //           &&  m1_strategy->AddTrader(TraderType::BACKTEST, "test", backtest_trader))
    //             LOG_INFO(LOGGER, "add backtest trade to strategy success");

            if(ctp_trader
               && m1_strategy->AddTrader(TraderType::CTP, "ctp", ctp_trader))
                 LOG_INFO(LOGGER, "add ctp trade to strategy success");

            if(stock_trader
               && m1_strategy->AddTrader(TraderType::BACKTEST, "stock", stock_trader))
                 LOG_INFO(LOGGER, "add stock trade to strategy success");

            if(ctp_tick_publisher
               && m1_strategy->AddEventPublisher(MessageType::CTPTICKDATA, "ctp_tick", ctp_tick_publisher))
                 LOG_INFO(LOGGER, "add ctp tick publisher to strategy success");

            if(bar_publisher
               && m1_strategy->AddEventPublisher(MessageType::CTPKBARDATA, "bar", bar_publisher))
                 LOG_INFO(LOGGER, "add bar publisher to strategy success");

            m1_strategy->Init();]]


            m1_strategy->Start();

            //m1 end
            */

            // kw start

            auto kw_strategy = std::make_shared<KWeightStrategy>(dataCenterStrategy, monitor);


            if(kw_strategy == nullptr)
            {
                 LOG_ERROR(LOGGER, "kw_strategy is null");
            }

//        if(backtest_trader
//           &&  m1_strategy->AddTrader(TraderType::BACKTEST, "test", backtest_trader))
//             LOG_INFO(LOGGER, "add backtest trade to strategy success");

            if(ctp_trader
               && kw_strategy->AddTrader(TraderType::CTP, "ctp", ctp_trader))
                 LOG_INFO(LOGGER, "add ctp trade to strategy success");

            if(stock_trader
               && kw_strategy->AddTrader(TraderType::BACKTEST, "stock", stock_trader))
                 LOG_INFO(LOGGER, "add stock trade to strategy success");

            if(ctp_tick_publisher
               && kw_strategy->AddEventPublisher(MessageType::CTPTICKDATA, "ctp_tick", ctp_tick_publisher))
                 LOG_INFO(LOGGER, "add ctp tick publisher to strategy success");

            if(bar_publisher
               && kw_strategy->AddEventPublisher(MessageType::CTPKBARDATA, "bar", bar_publisher))
                 LOG_INFO(LOGGER, "add bar publisher to strategy success");


            kw_strategy->Init();

            kw_strategy->Start();

            // kw end

//            if (!real_mode)
//            {
//                back_test_engine->StartMatchTaskThread();
//                ctp_tick_publisher->StartPushMarketDataThread();
//
//            }

            //m1_strategy->JoinPullThread();

            kw_strategy->JoinPullThread();

//            if (!real_mode)
//            {
//                ctp_tick_publisher->JoinPushMarketDataThread();
//                printf("push market data finish\n");
//
//                back_test_engine->StopTask();
//                back_test_engine->JoinMatchTaskThread();
//                printf("match task finish\n");
//
//            }

//        auto k_bar_generate = std::make_shared<SohaDataCenterStrategy>("k_bar_generate","k_bar_generate.xml",SubscribeMode::NOTICE);
//
//        if(k_bar_generate == nullptr)
//        {
//             LOG_ERROR(LOGGER, "k_bar_generate is null");
//        }
//
//        if(ctp_trader
//           && k_bar_generate->AddTrader(TraderType::CTP, "ctp", ctp_trader))
//             LOG_INFO(LOGGER, "add ctp trade to k_bar_generate success");
//
//        if(ctp_tick_publisher
//           && k_bar_generate->AddEventPublisher(EventType::CTPTICK, "ctp_tick", ctp_tick_publisher))
//             LOG_INFO(LOGGER, "add ctp tick publisher to k_bar_generate success");
//
//        k_bar_generate->Init();
//
//        k_bar_generate->Start();
        }


        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds (10));
            fflush(stdout);
            // LOG_INFO(LOGGER, "main thread sleep");
        }




    }
    catch(const char * msg)
    {
         LOG_ERROR(LOGGER, msg);
    }
}


int LoadTradingPeriod() {
    // load  trading_period.xml
    std::unordered_map<std::string, std::string> product_trading_period_type;
    std::unordered_map<std::string, TradingPeriod> trading_periods_map_;

    boost::property_tree::ptree pt_trading_period;
    boost::property_tree::read_xml("trading_period.xml", pt_trading_period);
    auto root = pt_trading_period.get_child("trading_period");
    for (auto it = root.begin(); it != root.end(); ++it)
    {
        if (it->first == "product")
        {
            const auto & product_name = it->second.get<std::string>("<xmlattr>.name");
            const auto & period_type = it->second.get<std::string>("<xmlattr>.period_type");
            //printf("product name: %s, period type: %s\n", product_name.c_str(), period_type.c_str());
            product_trading_period_type.emplace(product_name, period_type);
        }
        else if (it->first == "period_type")
        {
            const auto periods = it->second;
            int period_seq;
            int start_;
            int end_;
            TradingPeriod tp;
            for (auto it2 = periods.begin(); it2 != periods.end(); ++it2)
            {
                if (it2->first == "<xmlattr>")
                {
                    tp.type = it2->second.get<std::string>("type");
                    tp.period_counts = it2->second.get<int>("counts");
                    //printf("period type: %s, counts: %d\n", tp.type.c_str(), tp.period_counts);
                }
                else if(it2->first == "period")
                {
                    period_seq = it2->second.get<int>("<xmlattr>.seq");
                    start_ = it2->second.get<int>("<xmlattr>.start");
                    end_ = it2->second.get<int>("<xmlattr>.end");
                    //printf("period seq: %d, start %d, end: %d\n", period_seq, start_, end_);
                    tp.periods.emplace_back(period_seq, std::make_pair(start_,end_));
                }
            }
            trading_periods_map_.emplace(tp.type, tp);
        }
        else if(it->first == "day_time")
        {
            const auto time_ = it->second.get<size_t>("<xmlattr>.key");
            GlobalData::g_day_miute_vector_.push_back(time_);
        }
    }
    for (const auto & p : product_trading_period_type)
    {
        try {
            GlobalData::g_product_trading_periods_[p.first] = trading_periods_map_.at(p.second);
        }
        catch (...)
        {
            printf("init product trading period error\n");
        }
    }

    printf("\n");
    for (const auto & p : GlobalData::g_product_trading_periods_)
    {
        printf("product: %s, trading period type: %s, counts: %d\n",
               p.first.c_str(), p.second.type.c_str(), p.second.period_counts);

        for (const auto &l : p.second.periods)
        {
            printf("seq: %d, start %d, end %d\n", l.first, l.second.first, l.second.second);
        }
        printf("\n");

    }

//    printf("\n");
//    for (auto t: GlobalData::g_day_miute_vector_)
//    {
//        printf("time key: %d\n", t);
//    }

    GlobalData::InitMinuteIndex();

    return 0;
}