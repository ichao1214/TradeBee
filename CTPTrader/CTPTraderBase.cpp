//
// Created by haichao.zhang on 2022/3/19.
//

#include "CTPTraderBase.h"


std::map<std::string, CThostFtdcInstrumentField> CTPTraderBase::inst_map_;
std::map<std::string, CThostFtdcDepthMarketDataField> CTPTraderBase::market_map_;

std::map<std::string, CThostFtdcInstrumentCommissionRateField>  CTPTraderBase::commi_map_;
std::map<std::string, CThostFtdcInstrumentMarginRateField> CTPTraderBase::margin_map_;

std::map<std::string, std::multimap<int, std::string>> CTPTraderBase::product_pre_open_intetest_map_;
std::map<std::string, std::pair<std::string, std::string>> CTPTraderBase::product_main_symbol_map_;

std::string CTPTraderBase::basic_data_base_dir;

//int CTPTraderBase::LoadTradingPeriod() {
//    // load  trading_period.xml
//    std::unordered_map<std::string, std::string> product_trading_period_type;
//    std::unordered_map<std::string, TradingPeriod> trading_periods_map_;
//
//    boost::property_tree::ptree pt_trading_period;
//    boost::property_tree::read_xml("trading_period.xml", pt_trading_period);
//    auto root = pt_trading_period.get_child("trading_period");
//    for (auto it = root.begin(); it != root.end(); ++it)
//    {
//        if (it->first == "product")
//        {
//            const auto & product_name = it->second.get<std::string>("<xmlattr>.name");
//            const auto & period_type = it->second.get<std::string>("<xmlattr>.period_type");
//            //printf("product name: %s, period type: %s\n", product_name.c_str(), period_type.c_str());
//            product_trading_period_type.emplace(product_name, period_type);
//        }
//        else if (it->first == "period_type")
//        {
//            const auto periods = it->second;
//            int period_seq;
//            int start_;
//            int end_;
//            TradingPeriod tp;
//            for (auto it2 = periods.begin(); it2 != periods.end(); ++it2)
//            {
//                if (it2->first == "<xmlattr>")
//                {
//                    tp.type = it2->second.get<std::string>("type");
//                    tp.period_counts = it2->second.get<int>("counts");
//                    //printf("period type: %s, counts: %d\n", tp.type.c_str(), tp.period_counts);
//                }
//                else if(it2->first == "period")
//                {
//                    period_seq = it2->second.get<int>("<xmlattr>.seq");
//                    start_ = it2->second.get<int>("<xmlattr>.start");
//                    end_ = it2->second.get<int>("<xmlattr>.end");
//                    //printf("period seq: %d, start %d, end: %d\n", period_seq, start_, end_);
//                    tp.periods.emplace_back(period_seq, std::make_pair(start_,end_));
//                }
//            }
//            trading_periods_map_.emplace(tp.type, tp);
//        }
//        else if(it->first == "day_time")
//        {
//            const auto time_ = it->second.get<size_t>("<xmlattr>.key");
//            GlobalData::g_day_miute_vector_.push_back(time_);
//        }
//    }
//    for (const auto & p : product_trading_period_type)
//    {
//        try {
//            GlobalData::g_product_trading_periods_[p.first] = trading_periods_map_.at(p.second);
//        }
//        catch (...)
//        {
//            printf("init product trading period error\n");
//        }
//    }
//
//    printf("\n");
//    for (const auto & p : GlobalData::g_product_trading_periods_)
//    {
//        printf("product: %s, trading period type: %s, counts: %d\n",
//               p.first.c_str(), p.second.type.c_str(), p.second.period_counts);
//
//        for (const auto &l : p.second.periods)
//        {
//            printf("seq: %d, start %d, end %d\n", l.first, l.second.first, l.second.second);
//        }
//        printf("\n");
//
//    }
//
////    printf("\n");
////    for (auto t: GlobalData::g_day_miute_vector_)
////    {
////        printf("time key: %d\n", t);
////    }
//
//    GlobalData::InitMinuteIndex();
//
//    return 0;
//}

void CTPTraderBase::InitMainSymbol() {
    // 昨持仓最大者为主力合约
    for_each(market_map_.begin(), market_map_.end(), [&](std::pair<std::string, CThostFtdcDepthMarketDataField> i)
    {
        const auto & product_id = inst_map_[i.first].ProductID;
        const auto & symbol = i.first;
        const auto & pre_open_interest = i.second.PreOpenInterest;
        printf("market: %s %s %s %f %f %f\n", symbol.c_str(), i.second.ExchangeID, product_id, i.second.LastPrice, i.second.PreClosePrice, pre_open_interest);
        product_pre_open_intetest_map_[product_id].emplace(pre_open_interest, symbol);
    });

    for_each(product_pre_open_intetest_map_.begin(), product_pre_open_intetest_map_.end(), [&](std::pair<std::string, std::multimap<int, std::string>> i)
    {
        printf("product pre open interest: %s ", i.first.c_str());
        std::string symbol1{};
        std::string symbol2{};
        for_each(i.second.rbegin(), i.second.rend(), [&](std::pair<int, std::string> j)
        {
            printf("%s->%d ", j.second.c_str(), j.first);
            if (symbol1.empty())
                symbol1.assign(j.second);
            else if (symbol2.empty())
                symbol2.assign(j.second);
        });
        if (symbol1.empty())
        {
            printf("product %s , main symbol err\n", i.first.c_str());

        }

        if (symbol2.empty())
        {
            symbol2 = symbol1;
            printf("product %s , only one symbol %s\n", i.first.c_str(), symbol1.c_str());
        }

        printf("\n");

        product_main_symbol_map_[i.first] = make_pair(symbol1, symbol2);


    });


    for_each(product_main_symbol_map_.begin(), product_main_symbol_map_.end(), [&](std::pair<std::string , std::pair<std::string, std::string>> i)
    {
        printf("product: %s, symbol1 %s, symbol2 %s\n", i.first.c_str(), i.second.first.c_str(), i.second.second.c_str());
    });
}


void CTPTraderBase::InitCTPSymbolsFromBinFile() {
    std::string inst_bin_file{basic_data_base_dir + '/' + trading_day_ + "_inst"};
    int total_inst;
    CThostFtdcInstrumentField * inst_info_buf = nullptr;

    binfile2buf(inst_bin_file, inst_info_buf, total_inst);

    for(int i = 0; i< total_inst; ++i)
    {
        const auto &inst_info_ = inst_info_buf[i];
        inst_map_.emplace(inst_info_.InstrumentID, inst_info_);

        SymbolField symbolField{};
        strcpy(symbolField.Symbol, inst_info_.InstrumentID);
        symbolField.ExchangeId = ExchangeIDConvert(inst_info_.ExchangeID);
        symbolField.PriceTick = inst_info_.PriceTick;
        symbolField.Multiple = inst_info_.VolumeMultiple;
        symbolField.MaxMarketOrderVolume = inst_info_.MaxMarketOrderVolume;
        symbolField.MinMarketOrderVolume = inst_info_.MinMarketOrderVolume;
        symbolField.MaxLimitOrderVolume = inst_info_.MaxLimitOrderVolume;
        symbolField.MinLimitOrderVolume = inst_info_.MinLimitOrderVolume;
        strcpy(symbolField.ProductID, inst_info_.ProductID);


        GlobalData::g_symbol_map_.emplace(inst_info_.InstrumentID, symbolField);
        GlobalData::g_symbol_trading_end_map_.emplace(inst_info_.InstrumentID, true);

        GlobalData::g_symbol_index_map_[inst_info_.InstrumentID] = i;

    }

    GlobalData::g_symbol_init_flag_.store(true);
    delete[] inst_info_buf;
}

void CTPTraderBase::InitCommiFromBinFile() {// init commi
    std::string commi_bin_file{basic_data_base_dir + '/' + trading_day_ + "_commi"};
    int total_commi;
    CThostFtdcInstrumentCommissionRateField *commi_buf = nullptr;
    binfile2buf(commi_bin_file, commi_buf,total_commi);
    for(int i = 0; i< total_commi; ++i)
    {
        const auto& commi_info_ = commi_buf[i];
        commi_map_.emplace(commi_info_.InstrumentID, commi_info_);
    }
    for_each(commi_map_.begin(), commi_map_.end(), [](std::pair<std::string, CThostFtdcInstrumentCommissionRateField> i)
    {
        printf("commi: %s %s %f %f %f %f %f %f\n", i.first.c_str(), i.second.InstrumentID,
               i.second.OpenRatioByMoney,i.second.OpenRatioByVolume,
               i.second.CloseRatioByMoney,i.second.CloseRatioByVolume,
               i.second.CloseTodayRatioByMoney, i.second.CloseTodayRatioByVolume);
    });
    delete[] commi_buf;
}

void CTPTraderBase::InitMarginFromBinFile() {// init margin
    std::string margin_bin_file{basic_data_base_dir + '/' + trading_day_ + "_margin"};
    CThostFtdcInstrumentMarginRateField *margin_buf = nullptr;
    int total_margin;
    binfile2buf(margin_bin_file, margin_buf, total_margin);
    for(int i = 0; i< total_margin; ++i)
    {
        const auto& margin_info_ = margin_buf[i];
        margin_map_.emplace(margin_info_.InstrumentID, margin_info_);
    }
    for_each(margin_map_.begin(), margin_map_.end(), [](std::pair<std::string, CThostFtdcInstrumentMarginRateField> i)
    {
        printf("margin: %s %s %f %f %f %f\n", i.first.c_str(), i.second.ExchangeID,
               i.second.LongMarginRatioByMoney, i.second.LongMarginRatioByVolume,
               i.second.ShortMarginRatioByMoney, i.second.ShortMarginRatioByVolume);
    });
    delete[] margin_buf;
}

void CTPTraderBase::InitMarketFromBinFile() {// init market
    std::string market_bin_file{basic_data_base_dir + '/' + trading_day_ + "_market"};
    CThostFtdcDepthMarketDataField *market_buf = nullptr;
    int total_market;
    binfile2buf(market_bin_file, market_buf, total_market);
    for(int i = 0; i< total_market; ++i)
    {
        const auto& market_info_ = market_buf[i];
        market_map_.emplace(market_info_.InstrumentID, market_info_);
    }
    delete[] market_buf;
}

std::shared_ptr<OrderField> CTPTraderBase::Buy(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy)
{
    return InsertLimitPriceOrder(symbol, BUY, OPEN, volume, price, strategy);
}

std::shared_ptr<OrderField> CTPTraderBase::Sell(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy)
{
    return InsertLimitPriceOrder(symbol, SELL, CLOSE, volume, price, strategy);
}

std::shared_ptr<OrderField> CTPTraderBase::Short(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy)
{
    return InsertLimitPriceOrder(symbol, SELL, OPEN, volume, price, strategy);
}

std::shared_ptr<OrderField> CTPTraderBase::Cover(std::string symbol, double price, int volume, std::shared_ptr<StrategyBase> strategy)
{
    return InsertLimitPriceOrder(symbol, BUY, CLOSE, volume, price, strategy);
}

bool CTPTraderBase::Cancel(std::shared_ptr<OrderField>  order, std::shared_ptr<StrategyBase>  strategy) {
    auto exchange_id = GlobalData::GetSymbolField(order->Symbol).ExchangeId;

    return CancelCtpOrder(ExchangeNameConvert(exchange_id), std::string{order->OrderRef}, order->OrderID, order->Symbol);
}


std::shared_ptr<CommiField> CTPTraderBase::getSymbolCommi(std::string& symbol)  {
    auto commiField = std::make_shared<CommiField>();
    auto it = commi_map_.find(symbol);
    if (it != commi_map_.end())
    {
        commiField->OpenRatioByMoney = it->second.OpenRatioByMoney;
        commiField->OpenRatioByVolume = it->second.OpenRatioByVolume;
        commiField->CloseRatioByMoney = it->second.CloseRatioByMoney;
        commiField->CloseRatioByVolume = it->second.CloseRatioByVolume;
        commiField->CloseTodayRatioByMoney = it->second.CloseTodayRatioByMoney;
        commiField->CloseTodayRatioByVolume = it->second.CloseTodayRatioByVolume;
    }
    else
    {
        auto p = GlobalData::g_symbol_map_[symbol].ProductID;
        auto it2 = commi_map_.find(p);
        if (it2 != commi_map_.end())
        {
            commiField->OpenRatioByMoney = it2->second.OpenRatioByMoney;
            commiField->OpenRatioByVolume = it2->second.OpenRatioByVolume;
            commiField->CloseRatioByMoney = it2->second.CloseRatioByMoney;
            commiField->CloseRatioByVolume = it2->second.CloseRatioByVolume;
            commiField->CloseTodayRatioByMoney = it2->second.CloseTodayRatioByMoney;
            commiField->CloseTodayRatioByVolume = it2->second.CloseTodayRatioByVolume;
        }
    }
//    printf("%s commi: %f %f %f %f %f %f\n", symbol.c_str(), commiField->OpenRatioByMoney, commiField->OpenRatioByVolume,
//           commiField->CloseRatioByMoney, commiField->CloseRatioByVolume,
//           commiField->CloseTodayRatioByMoney, commiField->CloseTodayRatioByVolume);

    return commiField;

}

std::shared_ptr<MarginField> CTPTraderBase::getSymbolMargin(std::string& symbol)  {

    auto marginFile = std::make_shared<MarginField>();
    auto it = margin_map_.find(symbol);
    if (it != margin_map_.end())
    {
        marginFile->LongMarginRatioByMoney = it->second.LongMarginRatioByMoney;
        marginFile->LongMarginRatioByVolume = it->second.LongMarginRatioByVolume;
        marginFile->ShortMarginRatioByMoney = it->second.ShortMarginRatioByMoney;
        marginFile->ShortMarginRatioByVolume = it->second.ShortMarginRatioByVolume;
    }
    else
    {
        auto p = GlobalData::g_symbol_map_[symbol].ProductID;
        auto it2 = margin_map_.find(p);
        if (it2 != margin_map_.end())
        {
            marginFile->LongMarginRatioByMoney = it->second.LongMarginRatioByMoney;
            marginFile->LongMarginRatioByVolume = it->second.LongMarginRatioByVolume;
            marginFile->ShortMarginRatioByMoney = it->second.ShortMarginRatioByMoney;
            marginFile->ShortMarginRatioByVolume = it->second.ShortMarginRatioByVolume;
        }
    }

    return marginFile;
}

std::pair<std::string, std::string> CTPTraderBase::getProductMainSymbol(std::string &Product) {


    return product_main_symbol_map_[Product];
}


