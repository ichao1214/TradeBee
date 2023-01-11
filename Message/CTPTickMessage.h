//
// Created by haichao.zhang on 2022/1/8.
//

#ifndef TRADEBEE_CTPTICKMESSAGE_H
#define TRADEBEE_CTPTICKMESSAGE_H
#include "MessageBase.h"
#include <string>
#include <cstring>
#include "ThostFtdcUserApiStruct.h"
#include "ThostFtdcUserApiDataType.h"
#include <memory>
#include <utility>
#include <fstream>
#include "pubilc.h"
#include "../StrategyBase/StrategyBase.h"

class CTPTickMessage: public MessageBase {
private:

public:
    CTPTickMessage(int id, MessageType type, std::string remark)
            : MessageBase(id, type, std::move(remark)) {

    }




    void set_msg_body(CThostFtdcDepthMarketDataField * marketDataField)
    {
        sprintf(msg_body.tick.symbol, "%s", marketDataField->InstrumentID);
        sprintf(msg_body.tick.trading_day, "%s", marketDataField->TradingDay);
        sprintf(msg_body.tick.exchange_id, "%s",
                ExchangeNameConvert(GlobalData::g_symbol_map_[marketDataField->InstrumentID].ExchangeId));

        auto multi = GlobalData::GetSymbolField(msg_body.tick.symbol).Multiple;
        // CTP 发布的郑商所tick行情，成交金额没有乘上合约乘数
        if(ExchangeIDConvert(msg_body.tick.exchange_id) == ExchangeID::CZCE)
        {
            msg_body.tick.amount = IsValid(marketDataField->Turnover) ? marketDataField->Turnover * multi :  0;
        }
        else
        {
            msg_body.tick.amount = IsValid(marketDataField->Turnover) ? marketDataField->Turnover :  0;
        }

        msg_body.tick.last_price = IsValid(marketDataField->LastPrice) ? marketDataField->LastPrice : 0.0;
        msg_body.tick.average_price = IsValid(marketDataField->AveragePrice) ? marketDataField->AveragePrice : msg_body.tick.last_price;
        msg_body.tick.pre_close_price = IsValid(marketDataField->PreClosePrice) ? marketDataField->PreClosePrice :  msg_body.tick.last_price;
        msg_body.tick.pre_open_interest = IsValid(marketDataField->PreOpenInterest) ? marketDataField->PreOpenInterest : 0.0;
        msg_body.tick.open_price = IsValid(marketDataField->OpenPrice) ? marketDataField->OpenPrice :  msg_body.tick.last_price;
        msg_body.tick.highest_price = IsValid(marketDataField->HighestPrice) ? marketDataField->HighestPrice :  msg_body.tick.last_price;
        msg_body.tick.close_price = IsValid(marketDataField->ClosePrice) ? marketDataField->ClosePrice :  msg_body.tick.last_price;
        msg_body.tick.pre_settle_price = IsValid(marketDataField->PreSettlementPrice) ? marketDataField->PreSettlementPrice :  msg_body.tick.last_price;
        msg_body.tick.volume = marketDataField->Volume;
        msg_body.tick.lowest_price = IsValid(marketDataField->LowestPrice) ? marketDataField->LowestPrice :  msg_body.tick.last_price;
        msg_body.tick.open_interest = IsValid(marketDataField->OpenInterest) ? marketDataField->OpenInterest :  msg_body.tick.pre_open_interest;

        msg_body.tick.pre_close_price = marketDataField->PreClosePrice;
        msg_body.tick.pre_open_interest = marketDataField->PreOpenInterest;
        sprintf(msg_body.tick.update_time, "%s.%03d", marketDataField->UpdateTime, marketDataField->UpdateMillisec);
        msg_body.tick.tick_time = TimeString2Int(msg_body.tick.update_time);

        msg_body.tick.upper_limit_price = marketDataField->UpperLimitPrice;
        msg_body.tick.lower_limit_price = marketDataField->LowerLimitPrice;

        msg_body.tick.bid_price[0] = IsValid(marketDataField->BidPrice1) ? marketDataField->BidPrice1 : 0;
        msg_body.tick.bid_price[1] = IsValid(marketDataField->BidPrice2) ? marketDataField->BidPrice2 : 0;
        msg_body.tick.bid_price[2] = IsValid(marketDataField->BidPrice3) ? marketDataField->BidPrice3 : 0;
        msg_body.tick.bid_price[3] = IsValid(marketDataField->BidPrice4) ? marketDataField->BidPrice4 : 0;
        msg_body.tick.bid_price[4] = IsValid(marketDataField->BidPrice5) ? marketDataField->BidPrice5 : 0;

        msg_body.tick.bid_volume[0] = marketDataField->BidVolume1;
        msg_body.tick.bid_volume[1] = marketDataField->BidVolume2;
        msg_body.tick.bid_volume[2] = marketDataField->BidVolume3;
        msg_body.tick.bid_volume[3] = marketDataField->BidVolume4;
        msg_body.tick.bid_volume[4] = marketDataField->BidVolume5;

        msg_body.tick.ask_price[0] =  IsValid(marketDataField->AskPrice1) ? marketDataField->AskPrice1 : 0;
        msg_body.tick.ask_price[1] =  IsValid(marketDataField->AskPrice2) ? marketDataField->AskPrice2 : 0;
        msg_body.tick.ask_price[2] =  IsValid(marketDataField->AskPrice3) ? marketDataField->AskPrice3 : 0;
        msg_body.tick.ask_price[3] =  IsValid(marketDataField->AskPrice4) ? marketDataField->AskPrice4 : 0;
        msg_body.tick.ask_price[4] =  IsValid(marketDataField->AskPrice5) ? marketDataField->AskPrice5 : 0;

        msg_body.tick.ask_volume[0] = marketDataField->AskVolume1;
        msg_body.tick.ask_volume[1] = marketDataField->AskVolume2;
        msg_body.tick.ask_volume[2] = marketDataField->AskVolume3;
        msg_body.tick.ask_volume[3] = marketDataField->AskVolume4;
        msg_body.tick.ask_volume[4] = marketDataField->AskVolume5;

    }

};


#endif //TRADEBEE_CTPTICKMESSAGE_H
