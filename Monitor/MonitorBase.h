//
// Created by haichao.zhang on 2022/4/13.
//

#ifndef TRADEBEE_MONITORBASE_H
#define TRADEBEE_MONITORBASE_H

#include <utility>

#include "../StrategyBase/StrategyBase.h"


class StrategyOrderTrade
{
private:
    std::shared_ptr<OrderField> order;
    std::shared_ptr<TradeField> trade;
    double stop_price;
    double trail_price;
    StopType stopType;

public:
    const std::shared_ptr<OrderField> &getOrder() const {
        return order;
    }

    const std::shared_ptr<TradeField> &getTrade() const {
        return trade;
    }

    double getStopPrice() const {
        return stop_price;
    }


    StopType getSt() const {
        return stopType;
    }

    void setSt(StopType st) {
        StrategyOrderTrade::stopType = st;
    }

    StrategyOrderTrade(StopType st,
                       std::shared_ptr<OrderField> orderField,
                       std::shared_ptr<TradeField> tradeField,
                       double sp,
                       double tp = 0.0): stopType(st), order(std::move(orderField)), trade(std::move(tradeField)), stop_price(sp), trail_price(tp)
    {

    }


    bool CheckStop(const std::shared_ptr<TICK>& tick)
    {

        bool need_stop = false;

        if (order->Direction == OrderDirection::BUY)                    // 多头持仓
        {

            if (stopType == StopType::TRAIL)
            {
                if (tick->bid_price[0] - trail_price - stop_price > 0)    // 对手价 - 移动价格 > 当前止损价格（上涨）
                {
                    stop_price = tick->bid_price[0] - trail_price;        // 上调止损价格
                    printf("symbol %s, trade id %s, up adjust stop price to %f\n", trade->Symbol, trade->TradeID, stop_price);
                }
            }


            if (tick->bid_price[0] - stop_price < 0)                       // 跌破止损价格
            {
                need_stop = true;
            }
        }
        else                                                            // 空头持仓
        {
            if (stopType == StopType::TRAIL)
            {
                if (tick->ask_price[0] + trail_price - stop_price < 0)    //   对手价 + 移动价格 < 当前止损价格(下跌) 更新移动止损价格
                {
                    stop_price = tick->ask_price[0] + trail_price;         // 下调止损价格
                    printf("symbol %s, trade id %s, down adjust stop price to %f\n", trade->Symbol, trade->TradeID, stop_price);
                }
            }


            if (tick->ask_price[0] - stop_price > 0)                      // 涨破止损价格
            {
                need_stop = true;
            }
        }

        return need_stop;


    }

    std::shared_ptr<StrategyBase> GetOrderStrategy()
    {
        return order->strategy.lock();
    }
};

typedef std::list<std::shared_ptr<StrategyOrderTrade>> SymbolOrderTradeDetailListType;


class MonitorBase : public StrategyBase{

public:

    explicit MonitorBase(StopType type = StopType::LIMIT,
                std::string name = "monitor",
                std::string config = "monitor.xml"):stopType(type),StrategyBase(std::move(name), std::move(config))
    {


        common_stop_order_paras_ = std::make_shared<StopOrderPara>(5,5);

    }

    void AppendStrategyTrade(const std::shared_ptr<OrderField> &order, const std::shared_ptr<TradeField> &trade)
    {
        printf("monitor append strategy order trade: ");
        trade->print();

        double stop_price_tick_;
        double trail_price_tick_;

        auto symbol_info = GlobalData::GetSymbolField(order->Symbol);


        double stop_price ;
        auto it = products_stop_order_paras_.find(symbol_info.ProductID);
        if (it != products_stop_order_paras_.end())
        {
            stop_price_tick_ = it->second->stop_price_tick;
            trail_price_tick_ = it->second->trail_price_tick;
        }
        else
        {
            stop_price_tick_ = common_stop_order_paras_->stop_price_tick;
            trail_price_tick_ = common_stop_order_paras_->trail_price_tick;
        }

        double trail_price = trail_price_tick_ * symbol_info.PriceTick;
        if (order->Direction == OrderDirection::BUY)
        {
            stop_price = trade->TradePrice - symbol_info.PriceTick * stop_price_tick_;
        }
        else
        {
            stop_price = trade->TradePrice + symbol_info.PriceTick * stop_price_tick_;
        }


        printf("stop price %f, trail price %f\n", stop_price, trail_price);

        auto sot = std::make_shared<StrategyOrderTrade>(stopType, order, trade, stop_price, trail_price);

        std::lock_guard<std::mutex> l(sots_mtx_);
        auto it2 = symbols_order_trades_map_.find(order->Symbol);
        if (it2 != symbols_order_trades_map_.end())
        {
            it2->second.emplace_back(sot);
        }
        else
        {
            SymbolOrderTradeDetailListType op_list_{sot};
            symbols_order_trades_map_.emplace(order->Symbol, op_list_);
        }

    }

    void AppendStrategyTradeByCloseID(int close_strategy_order_id)
    {
        std::lock_guard<std::mutex> l(sots_mtx_);
        auto it = close_order_map_.find(close_strategy_order_id);
        if (it != close_order_map_.end())
        {
            auto & sot = it->second;
            sot->setSt(StopType::STP);               // 市价平仓止损
            auto it2 = symbols_order_trades_map_.find(sot->getOrder()->Symbol);
            if (it2 != symbols_order_trades_map_.end())
            {
                it2->second.emplace_back(sot);
            }
            else
            {
                SymbolOrderTradeDetailListType op_list_{sot};
                symbols_order_trades_map_.emplace(sot->getOrder()->Symbol, op_list_);
            }
        }
    }

    void RemoveStrategyTrade()
    {

    }

    void Stop()
    {
        running.store(false);
    }

protected:
    StopType stopType;


    std::shared_ptr<StopOrderPara> common_stop_order_paras_;

    std::unordered_map<std::string, std::shared_ptr<StopOrderPara>> products_stop_order_paras_;

    std::mutex sots_mtx_;
    std::unordered_map<std::string, SymbolOrderTradeDetailListType> symbols_order_trades_map_;


    std::mutex close_order_mtx_;
    std::unordered_map<int, std::shared_ptr<StrategyOrderTrade>> close_order_map_;
};



#endif //TRADEBEE_MONITORBASE_H
