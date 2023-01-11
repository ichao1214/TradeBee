//
// Created by zhanghaichao on 2021/12/20.
//

#include "SohaDataCenterStrategy.h"
#include "logger/LogManager.h"
#include "../Message/CTPTickMessage.h"
#include "../Message/BarMessage.h"

#include <boost/property_tree/xml_parser.hpp>




void SohaDataCenterStrategy::OnTimer()
{
//    for(auto const & traders : m_traders)
//    {
//        printf("%d ", traders.first);
//        for (auto const & trader : traders.second)
//        {
//            printf("%s %x\n", trader.first.c_str(), trader.second.get());
//            if(trader.first == "ctp")
//                trader.second->Buy("ag2206", 5000, 1);
//        }
//    }
}

bool SohaDataCenterStrategy::Init() {
    LOG_INFO(LOGGER,"{} Init {}" , strategy_name, config);
    printf("%s Init %s\n", strategy_name.c_str(), config.c_str());
    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(config, pt);
    boost::property_tree::ptree symbols = pt.get_child("strategy.symbols");
    boost::property_tree::ptree strategy_mode = pt.get_child("strategy.mode");
    LOG_INFO(LOGGER,"strategy mode {}", strategy_mode.data());

    boost::property_tree::ptree mysql_host_pt = pt.get_child("strategy.mysql.host");
    boost::property_tree::ptree mysql_user_pt = pt.get_child("strategy.mysql.user");
    boost::property_tree::ptree mysql_password_pt = pt.get_child("strategy.mysql.password");
    LOG_INFO(LOGGER,"mysql host {}", mysql_host_pt.data());
    printf("mysql host %s\n", mysql_host_pt.data().c_str());
    LOG_INFO(LOGGER,"mysql user {}", mysql_user_pt.data());
    printf("mysql user %s\n", mysql_user_pt.data().c_str());

    boost::property_tree::ptree save2db_pt = pt.get_child("strategy.save2db");
    save2db = save2db_pt.data();

    // init default ctp trader
    if (m_traders.find(TraderType::CTP)!= m_traders.end() &&
            m_traders[TraderType::CTP].find("ctp") !=  m_traders[TraderType::CTP].end())
    {
        default_ctp_trader = m_traders[TraderType::CTP].at("ctp");
    }

    // init strategy_symbols
    if (default_ctp_trader != nullptr && strategy_mode.data() == "KBarGenerator")
    {
        if (!GlobalData::g_symbol_init_flag_ || GlobalData::g_symbol_map_.empty())
        {
            printf("init strategy symbol error\n");
        }
        strategy_symbols = default_ctp_trader->get_all_symbols();
        LOG_INFO(LOGGER,"KBarGenerator watch symbols {}", strategy_symbols.size());
        printf("KBarGenerator watch symbols %lu\n", strategy_symbols.size());
    }
    else
    {
        for(auto it = symbols.begin(); it != symbols.end(); ++it)
        {
           auto symbol_node = it->second;
            printf("%s append symbol %s\n", strategy_name.c_str(), symbol_node.data().c_str());
            LOG_INFO(LOGGER,"{} add {}", strategy_name, symbol_node.data());
            strategy_symbols.emplace_back(symbol_node.data().c_str());
        }
    }

    // init minute k bars series
    for (const auto & s : strategy_symbols)
    {
        auto minute_bar_series = std::make_shared<BarSeries>(BarIntervalType::MINUTE, s);
        minute_k_bar_series.emplace(s, minute_bar_series);
    }

    minute_index_size = GlobalData::g_minute_index_map_.size();

    symbol_index_size = GlobalData::g_symbol_index_map_.size();

    printf("minute index size: %lu, symbol index size: %lu\n", minute_index_size, symbol_index_size);

    bar_center.resize(minute_index_size);
    for (size_t m = 0; m < minute_index_size; ++m) {
       bar_center[m].resize(symbol_index_size);
    }

    for (size_t m = 0; m < minute_index_size; ++m)
    {
        for (size_t s = 0; s < symbol_index_size; ++s)
        {
            bar_center[m][s] = nullptr;
        }
    }


    // init mysql helper
    mySqlHelper = std::make_shared<MySqlHelper>(mysql_host_pt.data(), mysql_user_pt.data(), mysql_password_pt.data());
    mysql_conn = mySqlHelper->InitMysqlClientConn();


    size_t ret_count = 0;
    auto start_day = GlobalData::g_trading_day_;
    auto end_day = GlobalData::g_trading_day_;

    if (strategy_mode.data() == "KBarGenerator")
        ret_count = LoadMinuteBarFromDB(start_day, end_day, {});
    else
        ret_count = LoadMinuteBarFromDB(start_day,end_day, strategy_symbols);
    printf("load bar from %s to %s count %lu\n", start_day.c_str(), end_day.c_str(), ret_count);

    // init ctp tick publisher
//    if (m_publishers.find(MessageType::CTPTICKDATA)!= m_publishers.end() &&
//        m_publishers[MessageType::CTPTICKDATA].find("ctp_tick") != m_publishers[MessageType::CTPTICKDATA].end())
//    {
//        default_ctp_tick_publisher = m_publishers[MessageType::CTPTICKDATA].at("ctp_tick");
//    }


    Subscribe(MessageType::CTPTICKDATA, strategy_symbols);



    return true;
}

void SohaDataCenterStrategy::Start() {
    LOG_INFO(LOGGER,"{} Start", strategy_name);
    printf("%s Start\n", strategy_name.c_str());
    RegisterTimer(5000);

//    if (default_ctp_tick_publisher != nullptr)
//    {
//        default_ctp_tick_publisher->Subscribe(MessageType::CTPTICKDATA, strategy_symbols);
//    }
//    else
//    {
//        LOG_ERROR(LOGGER,"default publisher is null");
//    }
}

void SohaDataCenterStrategy::OnTick(const shared_ptr<TICK> &tick) {
//    printf("Soha OnTick: ");
//    tick->print();
    UpdateBar(tick);
}

//void SohaDataCenterStrategy::Subscribe(MessageType type, std::vector<std::string> symbols) {
//    if (type==MessageType::CTPKBARDATA)
//    {
//        notice_symbols = symbols;
//    }
//    else
//    {
//        LOG_ERROR(LOGGER,"not my job");
//    }
//}

void SohaDataCenterStrategy::UpdateBar(const std::shared_ptr<TICK> &tick) {
    auto md_minute_ = Adj2Min(tick->tick_time);
//    printf("%s UpdateBar %s %s %s: tick_time: %d, min: %d, last_price: %.2lf, open_price: %.2lf, high_price: %.2lf, "
//           "low_price: %.2lf, close_price: %.2lf, ""volume: %d, open_interest: %.2lf, average_price: %.2lf, "
//                                                   "pre open interest: %f, pre close %f, amount %f\n",
//           strategy_name.c_str(), tick->exchange_id, tick->update_time,tick->symbol,tick->tick_time,
//           md_minute_, tick->last_price, tick->open_price,tick->highest_price,
//           tick->lowest_price,tick->close_price, tick->volume, tick->open_interest, tick->average_price,
//           tick->pre_open_interest, tick->pre_close_price, tick->amount);
    auto symbol_info = GlobalData::GetSymbolField(tick->symbol);
    auto trading_period = GlobalData::GetProductTradingPeriods(symbol_info.ProductID);
    //printf("period type: %s, count: %d\n", trading_period.type.c_str(), trading_period.period_counts);
    int period_seq_ = -1;    // 交易阶段 从 1 开始
    int first_start = -1;    // 开盘时间
    int last_end = -1;       // 收盘时间
    bool period_enf_flag = true;   // 交易阶段结束标记, 合约没有开盘也算交易结束
//    printf("md_minute %d\n",
//           md_minute_);
    for (int i = 0; i < trading_period.period_counts; ++i)
    {
        const auto & tp = trading_period.periods[i];
//        printf("seq %d, start %d, end %d, md_minute %d\n",
//               tp.first, tp.second.first, tp.second.second, md_minute_);
        auto start = tp.second.first;         // 当前交易时段开始时间点（包含）
        auto end = tp.second.second;          // 当前交易时段结束时间点（不包含）

        if (tp.first == 1)
            first_start = start;              // 开盘时间
        if (tp.first == trading_period.period_counts)
            last_end = end;                   // 收盘时间

        if (end == 0)       // 0 点换算成24点
            end = 240000;

        if(md_minute_ >= start && md_minute_ <= end)  // 当前时间属于配置的某个交易时间段范围
        {
            period_seq_ = tp.first;
            period_enf_flag = false;                // 交易中

            if (md_minute_ == end)                  // 当前时间等于结束时间，视为交易阶段结束（压线）
                period_enf_flag = true;

            if ( //(ExchangeIDConvert(tick->exchange_id) == ExchangeID::CZCE
                 // || ExchangeIDConvert(tick->exchange_id) == ExchangeID::DCE
                 // || ExchangeIDConvert(tick->exchange_id) == ExchangeID::INE)
                // &&
                    //&& period_seq_ ==  trading_period.period_counts
                 //&& end * 1000 - tick->tick_time <= 4041000)    // 郑商所(大商所有时也会)收盘（夜盘结束）没有 xx:00:00 的行情， xx:59:59 视为交易阶段结束
                                                                 // 23000000 - 225959000 =  4041000， 15000000 - 145959000 =  4041000
                                                                 // 离谱的郑商所行情，有的时候22：59：58 就没有行情推送了
                                                                 // nr 20号胶（上期能源）也出现了没有23：00：00行情的情况
                  (tick->tick_time >= 225950000 && tick->tick_time <= 225959999
                   ||tick->tick_time >= 5900000 && tick->tick_time <= 5959999
                   ||tick->tick_time >= 101450000 && tick->tick_time <= 101459999
                   ||tick->tick_time >= 112950000 && tick->tick_time <= 112959999
                   ||tick->tick_time >= 145950000 && tick->tick_time <= 145959999 ) )
            {
                period_enf_flag = true;
            }

            break;

        }
        else if (i >=0
                 && i + 1 < trading_period.period_counts                          // 不是最后一个交易阶段
                 && md_minute_ > end                                              // 且当前时间大于当前阶段的结束时间
                 && md_minute_ < trading_period.periods[i+1].second.first)        // 且小于下一个交易阶段的开始时间
        {
            period_seq_ = tp.first;                                                // 交易阶段算当前交易阶段
            md_minute_ = trading_period.periods[i+1].second.first;                 // next start 时间转为下一个阶段的开始
            period_enf_flag = false;                                               // 交易中
            printf("pause time md ");
            tick->print();
            break;
        }
    }

    if (period_seq_ == -1)                                                       // 当前时间不属于配置的交易时间段
    {
        if (abs(md_minute_ - first_start) < abs(md_minute_ - last_end))    // 当前时间更接近开盘时间还是收盘时间
        {                                                                        // 接近开盘
            if (first_start - md_minute_ <= 4100)   // 210000-205900 = 4100,  90000 - 85900 = 4100, 夜盘早盘开盘前一分钟视为第一阶段
            {
                md_minute_ = first_start;
                period_seq_ = 1;
                period_enf_flag = false;                   // 交易中
            }
            else                                      // 丢弃
            {
                //printf("drop md minute %d\n", md_minute_);
                return;
            }
        }
        else                                                                      // 接近收盘
        {
            if (md_minute_ - last_end <= 0)                                       // 死代码，走不到
            {
                printf("amazing ");
                tick->print();
                md_minute_ = last_end;
                period_seq_ = trading_period.period_counts;
                period_enf_flag = true;
            }
            else                                                                   // 大于收盘时间 丢弃
            {
                //printf("drop md minute %d\n", md_minute_);
                return;
            }
        }
    }
//
//    if ( strcmp(tick->symbol,"AP210")==0) tick->print();
//     //更新合约交易状态
//    if (period_enf_flag)
//    {
//        printf("period end ");
//        tick->print();
//    }

    GlobalData::g_symbol_trading_end_map_[tick->symbol] = period_enf_flag;

//
//    if ( ExchangeIDConvert(tick->exchange_id) == ExchangeID::CZCE
//        && period_seq_ ==  trading_period.period_counts
//        && last_end * 1000 - tick->tick_time <= 4041000)    // 郑商所 没有 15:00:00 的行情， 14:59:59 视为结束
//                                                            // 15000000 - 145959000 =  4041000
//    {
//        period_enf_flag = true;
//    }
//    printf("update time: %s, tick time: %d, md_minute: %d, symbol: %s, exchange:%s , period seq: %d, period end: %s\n",
//           tick->update_time, tick->tick_time, md_minute_, tick->symbol, tick->exchange_id, period_seq_, period_enf_flag?"true":"false");
//    tick->print();


    /// Todo calculate bar
    std::shared_ptr<BAR> close_bar = nullptr;
    auto it = minute_k_bar_series.find(tick->symbol);
    if (it == minute_k_bar_series.end())
    {
        auto bar_series = std::make_shared<BarSeries>(BarIntervalType::MINUTE , tick->symbol);
        close_bar = bar_series->UpdateBarSeries(md_minute_, tick, period_seq_, period_enf_flag);

        minute_k_bar_series[tick->symbol] = bar_series;
    }
    else
    {
        auto bar_series = it->second;
        close_bar = bar_series->UpdateBarSeries(md_minute_, tick, period_seq_, period_enf_flag);
    }

    if (close_bar)
    {
        int symbol_bar_seq = IncreaseSymbolBarSeq(close_bar);
        //printf("symbol bar seq %d ", symbol_bar_seq);
        //close_bar->print();

        UpdateBarCenter(close_bar);

        auto msg = std::make_shared<BarMessage>(++bar_seq, MessageType::CTPKBARDATA, "bar");
        msg->set_msg_body(close_bar);
        PushOrNoticeMsgToAllSubscriber(msg);

        if (save2db == "1")
        {
            SaveMinuteBarToDB(symbol_bar_seq, close_bar);
        }
    }
}

int SohaDataCenterStrategy::IncreaseSymbolBarSeq(const shared_ptr<BAR> &close_bar) {
    auto it = symbol_last_bar_time_map_.find(close_bar->symbol);
    if (it != symbol_last_bar_time_map_.end())
    {
        auto symbol_last_bar_time = it->second;
        if (symbol_last_bar_time != close_bar->bar_time)
        {
            symbol_last_bar_time_map_[close_bar->symbol] = close_bar->bar_time;
            ++ symbol_last_bar_seq_map_[close_bar->symbol];
        }
    }
    else
    {
        symbol_last_bar_time_map_[close_bar->symbol] = close_bar->bar_time;
        symbol_last_bar_seq_map_[close_bar->symbol] = 0;
    }
    auto  symbol_bar_seq = symbol_last_bar_seq_map_[close_bar->symbol];
    return symbol_bar_seq;
}

bool SohaDataCenterStrategy::SaveMinuteBarToDB(int symbol_bar_seq, const shared_ptr<BAR> &bar) {

    try
    {
        char sql[1024]{};
        sprintf(sql, "replace into minute_bars "
                     "( symbol_bar_seq, symbol, trading_day, bar_time, "
                     "open, high, low, close, volume, start_volume, amount, start_amount, "
                     "average, price_change, price_change_pct, price_amplitude, "
                     "change_open_interest, last_volume, last_amount, day_average, "
                     "start_open_interest, open_interest, pre_close, period ) values "
                     "( %d, '%s', %zu, %zu, "
                     "%.3f, %.3f, %.3f, %.3f, %d, %d, %.3f, %.3f, "
                     "%f, %f, %f, %f, "
                     "%f, %d, %.3f, %f, "
                     "%f, %f, %.3f, %d)",
                     symbol_bar_seq, bar->symbol, bar->bar_trading_day, bar->bar_time,
                     bar->open, bar->high, bar->low, bar->close, bar->volume, bar->start_volume, bar->amount, bar->start_amount,
                     bar->average, bar->price_change, bar->price_change_pct, bar->price_amplitude,
                     bar->change_open_interest, bar->last_volume, bar->last_amount, bar->average_price_daily,
                     bar->start_open_interest, bar->open_interest, bar->pre_close, bar->period);
        //printf("sql: %s\n", sql);
        auto prep_stmt = mysql_conn->createStatement();
        auto update_count = prep_stmt->executeUpdate(sql);
        //printf("save %d bar to db success\n", update_count);
        mysql_conn->commit();
        prep_stmt->close();
    }
    catch (const std::exception& e)
    {
        printf("save bar to db failed: %s\n", e.what());
        return false;
    }
    return true;
}

size_t SohaDataCenterStrategy::LoadMinuteBarFromDB(const std::string& start_days,
                                                   const std::string& end_day,
                                                   const std::vector<std::string>& symbols)
{
    size_t ret_count = 0;
    if (mysql_conn)
    {
        auto stmt = mysql_conn->createStatement();

        std::string sql = "select symbol_bar_seq, symbol, trading_day, bar_time, "
                     "open, high, low, close, volume, start_volume, amount, start_amount, "
                     "average, price_change, price_change_pct, price_amplitude, "
                     "change_open_interest, last_volume, last_amount, day_average, "
                     "start_open_interest, open_interest, pre_close, period from minute_bars where 1=1 ";
        if (!start_days.empty())
        {
            sql +=  "and trading_day>=" + start_days + " ";
        }
        if (!end_day.empty())
        {
            sql += "and trading_day<=" + end_day + " ";
        }
        if (!symbols.empty())
        {
            sql = sql + "and symbol in (";
            for (const auto& s: symbols)
            {
                sql += "'" +  s + "',";
            }
            sql.pop_back();
            sql += ")";
        }

        sql += " order by symbol_bar_seq, symbol";
        printf("query sql: %s\n", sql.c_str());

        auto rs = stmt->executeQuery(sql);

        while (rs->next()) {
            ret_count++;

//            printf("%-12d\t%-12s\t%-12d\t%-12d\t%-9.3Lf\t%-9.3Lf\t%-9.3Lf\t%-9.3Lf\t%-12d\t%-19.3Lf\t%-19.3Lf\t%-9.3Lf\t"
//                   "%-9.3Lf\t%-9.3Lf\t%-9.3Lf\t%-12d\t%-19.3Lf\t%-9.3Lf\t%-9.3Lf\n",
//                   rs->getInt("symbol_bar_seq"),
//                   rs->getString("symbol").c_str(),
//                   rs->getInt("trading_day"),
//                   rs->getInt("bar_time"),
//                   rs->getDouble("open"),
//                   rs->getDouble("high"),
//                   rs->getDouble("low"),
//                   rs->getDouble("close"),
//                   rs->getInt("volume"),
//                   rs->getDouble("amount"),
//                   rs->getDouble("average"),
//                   rs->getDouble("price_change"),
//
//                   rs->getDouble("price_change_pct"),
//                   rs->getDouble("price_amplitude"),
//                   rs->getDouble("change_open_interest"),
//                   rs->getInt("last_volume"),
//                   rs->getDouble("last_amount"),
//                   rs->getDouble("day_average"),
//                   rs->getDouble("open_interest")
//            );

            auto load_bar = std::make_shared<BAR>();
            strcpy(load_bar->symbol, rs->getString("symbol").c_str());
            auto bar_time =  rs->getInt("bar_time");
            load_bar->bar_time = bar_time;
            load_bar->bar_trading_day = rs->getInt("trading_day");
            load_bar->open = rs->getDouble("open");
            load_bar->high = rs->getDouble("high");
            load_bar->low = rs->getDouble("low");
            load_bar->close = rs->getDouble("close");
            load_bar->volume = rs->getInt("volume");
            load_bar->start_volume = rs->getInt("start_volume");
            load_bar->amount = rs->getDouble("amount");
            load_bar->start_amount = rs->getDouble("start_amount");
            load_bar->average = rs->getDouble("average");
            load_bar->price_change = rs->getDouble("price_change");
            load_bar->price_change_pct = rs->getDouble("price_change_pct");
            load_bar->price_amplitude = rs->getDouble("price_amplitude");
            load_bar->change_open_interest = rs->getDouble("change_open_interest");
            load_bar->last_volume = rs->getInt("last_volume");
            load_bar->last_amount = rs->getDouble("last_amount");
            load_bar->pre_close = rs->getDouble("pre_close");
            load_bar->average_price_daily = rs->getDouble("day_average");
            load_bar->open_interest = rs->getDouble("open_interest");
            load_bar->start_open_interest = rs->getDouble("start_open_interest");
            load_bar->period = rs->getInt("period");


            minute_k_bar_series[load_bar->symbol]->k_bars.push_back(load_bar);
            minute_k_bar_series[load_bar->symbol]->k_bars_map_.emplace(bar_time, load_bar);
            minute_k_bar_series[load_bar->symbol]->k_bars_time_keys_.push_back(bar_time);
            minute_k_bar_series[load_bar->symbol]->last_period_seq_ = load_bar->period;

            IncreaseSymbolBarSeq(load_bar);

            UpdateBarCenter(load_bar);

//            auto msg = std::make_shared<BarMessage>(++bar_seq, MessageType::CTPKBARDATA, "bar");
//            msg->set_msg_body(load_bar);
//            PushOrNoticeMsgToAllSubscriber(msg);

        }
        rs->close();
        delete rs;
        stmt->close();
        delete stmt;
    }
    return ret_count;
}

void SohaDataCenterStrategy::UpdateBarCenter(shared_ptr<BAR> &load_bar) {
    auto minute_index_ = GlobalData::GetMinuteIndex(load_bar->bar_time);
    auto symbol_index_ = GlobalData::GetSymbolIndex(load_bar->symbol);

    bar_center[minute_index_][symbol_index_] = load_bar;
}


//std::shared_ptr<BAR> SohaDataCenterStrategy::GetBar(size_t minute_index, size_t symbol_index) {
//
//    auto bar = bar_center[minute_index][symbol_index];
//    return bar ? bar : nullptr ;
//}

//std::shared_ptr<BAR> SohaDataCenterStrategy::GetBar(size_t minute_time, const std::string& symbol) {
//
//    auto minute_index = GlobalData::GetMinuteIndex(minute_time);
//    auto symbol_index = GlobalData::GetSymbolIndex(symbol);
//    return GetBar(minute_index, symbol_index);
//}

void SohaDataCenterStrategy::UpdateSymbolTradeCost(const std::string& symbol, double price) {
    //printf("UpdateSymbolTradeCost\n");
    std::lock_guard<std::mutex> l(trade_cost_mtx_);
    auto cost =  CalSymbolTradeCost(symbol, price);
    symbol_trade_costs_map_[symbol] = cost;
//    printf("%s per const: long margin %.2lf, short margin %.2lf, open commi %.2lf, close commi %.2lf, close today commi %.2lf\n",
//           symbol.c_str(), cost->LongMarginCostPer, cost->ShortMarginCostPer, cost->OpenCostPer, cost->CloseCostPer, cost->CloseTodayCostPer);

}

std::shared_ptr<DAYSYMBOLMINUTEBARS> SohaDataCenterStrategy::getBarCenter() {
    return std::shared_ptr<DAYSYMBOLMINUTEBARS>(&bar_center);
}






