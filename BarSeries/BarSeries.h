//
// Created by haichao.zhang on 2022/3/17.
//

#ifndef TRADEBEE_BARSERIES_H
#define TRADEBEE_BARSERIES_H

#include "../Message/MessageBase.h"
#include "../StrategyBase/StrategyBase.h"
#include <list>
#include <unordered_map>
#include <map>



class BarSeries {
public:
    BarIntervalType type;
    std::string symbol;
    std::vector<std::shared_ptr<BAR>> k_bars;
    std::vector<int> k_bars_time_keys_;
    std::unordered_map<unsigned int, std::shared_ptr<BAR>> k_bars_map_;
    int last_period_seq_;
protected:
public:
    BarSeries(BarIntervalType type, std::string symbol):type(type),symbol(symbol)
    {
        k_bars.clear();
        k_bars_map_.clear();
        last_period_seq_ = -1;
    }

    ~BarSeries()
    {
        k_bars.erase(k_bars.begin(), k_bars.end());
        k_bars_map_.erase(k_bars_map_.begin(), k_bars_map_.end());
    }


    std::shared_ptr<BAR> UpdateBarSeries(int time_key, const std::shared_ptr<TICK> &tick, int period_seq_, bool period_end_flag)
    {
        std::shared_ptr<BAR> close_bar = nullptr;

        std::shared_ptr<BAR> bar = nullptr;
        if (period_seq_<last_period_seq_)
        {
//            printf("period err: symbol %s, time key %d, last %d, new %d\n",
//                   tick->symbol, time_key, last_period_seq_, period_end_flag);
            return nullptr;
        }

        auto it = k_bars_map_.find(time_key);
        if (it!= k_bars_map_.end() || period_end_flag)
        {
            if (it != k_bars_map_.end())   // find
            {
                bar = it->second;
            }
            else if (period_end_flag)     // not find but the last tick of this period
            {
                try {
                    if (!k_bars_time_keys_.empty())
                    {
                        auto last_bar_time_key = k_bars_time_keys_.back();
//                        // check tick time
//                        printf("period end, time key %d, tick time %d, last bar time %d\n",
//                               time_key, tick->tick_time, last_bar_time_key);
                        bar = k_bars_map_.at(last_bar_time_key);
                    }
                    else
                    {
                        throw "k bars time key empty!";
                    }
                }
                catch (...) {
//                    printf("get last bar fail: %s %d\n", tick->symbol, time_key);
//                    tick->print();
                    return nullptr;
                }
            }
            // update
            bar->high = tick->last_price> bar->high ? tick->last_price : bar->high;
            bar->low = tick->last_price< bar->low ? tick->last_price : bar->low;
            bar->close = tick->last_price;

            bar->last_volume = tick->volume;
            bar->volume = bar->last_volume - bar->start_volume;

            bar->open_interest = tick->open_interest;
            bar->change_open_interest = tick->open_interest - bar->start_open_interest;

            bar->last_amount = tick->amount;
            bar->amount = bar->last_amount - bar->start_amount;

            bar->price_change = bar->close - bar->pre_close;
            bar->price_change_pct = bar->price_change / bar->pre_close;
            bar->price_amplitude = bar->high / bar->low - 1 ;

            auto multi = GlobalData::GetSymbolField(bar->symbol).Multiple;
            bar->average = (bar->volume != 0) ?(bar->amount / bar->volume/ multi) : 0;
            bar->average_price_daily = (bar->last_volume != 0) ?(bar->last_amount / bar->last_volume/ multi) : 0;

        }
        else                 // not find time key and not the last tick of this period
        {
            int last_volume = 0;
            double last_amount = 0.0;
            double last_open_interest = tick->pre_open_interest;
            double pre_close = tick->pre_close_price;
            // pop pre bar ?
            if (!k_bars_map_.empty())
            {

                auto last_bar_time_key = k_bars_time_keys_.back();
                try {
                    auto last_bar = k_bars_map_.at(last_bar_time_key);
                    close_bar = last_bar;
//                    printf("bar close: symbol %s, trading day %d, bar_time %d, open %f, high %f, low %f, close %f, "
//                           "volume %d, change open interest %f, open interest %f, price change %f, price change pct %f, "
//                           "price amplitude %f, pre close %f, amount %f, average %f, average price daily %f, last amount %f\n",
//                           close_bar->symbol, close_bar->bar_trading_day, close_bar->bar_time,
//                           close_bar->open, close_bar->high, close_bar->low, close_bar->close, close_bar->volume,
//                           close_bar->change_open_interest, close_bar->open_interest, close_bar->price_change,
//                           close_bar->price_change_pct, close_bar->price_amplitude, close_bar->pre_close,
//                           close_bar->amount, close_bar->average, close_bar->average_price_daily, close_bar->last_amount);

                    last_volume = last_bar->last_volume;
                    last_amount = last_bar->last_amount;
                    last_open_interest = last_bar->open_interest;
                    pre_close = last_bar->close;
                }
                catch (...) {
                    //printf("get last bar fail\n");
                    return nullptr;
                }
            }
            // new bar
            auto new_bar =  std::make_shared<BAR>();
            strcpy(new_bar->symbol, tick->symbol);
            new_bar->bar_trading_day = std::stoi(tick->trading_day);
            new_bar->bar_time = time_key;
            new_bar->open = tick->last_price;
            new_bar->high = tick->last_price;
            new_bar->low = tick->last_price;
            new_bar->close = tick->last_price;
            new_bar->pre_close = (pre_close != 0) ? pre_close : new_bar->open;

            //new_bar->volume = 0;
            new_bar->last_volume = tick->volume;
            new_bar->start_volume = last_volume;
            new_bar->volume = new_bar->last_volume - new_bar->start_volume;

            new_bar->open_interest = tick->open_interest;
            new_bar->start_open_interest = last_open_interest;
            //new_bar->change_open_interest = 0;
            new_bar->change_open_interest = new_bar->open_interest - new_bar->start_open_interest;

            //new_bar->amount = 0;
            new_bar->last_amount = tick->amount;
            new_bar->start_amount = last_amount;
            new_bar->amount = new_bar->last_amount - new_bar->start_amount;

            new_bar->price_change = new_bar->close - new_bar->pre_close;
            new_bar->price_change_pct = new_bar->price_change / new_bar->pre_close;
            new_bar->price_amplitude = new_bar->high / new_bar->low - 1 ;
            new_bar->average = 0;
            new_bar->average_price_daily = 0;
            new_bar->period = period_seq_;


            k_bars.push_back(new_bar);
            k_bars_map_.emplace(time_key, new_bar);
            k_bars_time_keys_.push_back(time_key);
            last_period_seq_ = period_seq_ > last_period_seq_ ? period_seq_ : last_period_seq_;


        }

        if (period_end_flag)
        {
            close_bar = bar;
//            printf("period end, bar close: symbol %s, trading day %d, bar_time %d, open %f, high %f, low %f, close %f, "
//                   "volume %d, change open interest %f, open interest %f, price change %f, price change pct %f, "
//                   "price amplitude %f, pre close %f, amount %f, average %f\n",
//                   close_bar->symbol, close_bar->bar_trading_day, close_bar->bar_time,
//                   close_bar->open, close_bar->high, close_bar->low, close_bar->close, close_bar->volume,
//                   close_bar->change_open_interest, close_bar->open_interest, close_bar->price_change,
//                   close_bar->price_change_pct, close_bar->price_amplitude, close_bar->pre_close, close_bar->amount, close_bar->average);
        }

        return close_bar;
    }

};


#endif //TRADEBEE_BARSERIES_H
