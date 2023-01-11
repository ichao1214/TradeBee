//
// Created by haichao.zhang on 2022/1/27.
//


#include "TradeBeeDataStruct.h"
#include <cstring>

extern "C"
{

ExchangeID ExchangeIDConvert(const char *exchange) {
    if (strcmp(exchange, "SHFE") == 0) {
        return ExchangeID::SHFE;
    } else if (strcmp(exchange, "CZCE") == 0) {
        return ExchangeID::CZCE;
    } else if (strcmp(exchange, "DCE") == 0) {
        return ExchangeID::DCE;
    } else if (strcmp(exchange, "CFFEX") == 0) {
        return ExchangeID::CFFEX;
    } else if (strcmp(exchange, "INE") == 0) {
        return ExchangeID::INE;
    } else if (strcmp(exchange, "SGE") == 0) {
        return ExchangeID::SGE;
    } else if (strcmp(exchange, "SSE") == 0) {
        return ExchangeID::SSE;
    } else if (strcmp(exchange, "SZSE") == 0) {
        return ExchangeID::SZSE;
    } else if (strcmp(exchange, "BSE") == 0) {
        return ExchangeID::BSE;
    }
    return ExchangeID::SHFE;
}

const char *ExchangeNameConvert(ExchangeID exchangeId) {
    const char *exchange_name = "";
    switch (exchangeId) {
        case ExchangeID::SHFE :
            exchange_name = "SHFE";
            break;
        case ExchangeID::CZCE :
            exchange_name = "CZCE";
            break;
        case ExchangeID::DCE :
            exchange_name = "DCE";
            break;
        case ExchangeID::CFFEX :
            exchange_name = "CFFEX";
            break;
        case ExchangeID::INE :
            exchange_name = "INE";
            break;
        case ExchangeID::SGE :
            exchange_name = "SGE";
            break;
        case ExchangeID::SSE :
            exchange_name = "SSE";
            break;
        case ExchangeID::SZSE :
            exchange_name = "SZSE";
            break;
        case ExchangeID::BSE :
            exchange_name = "BSE";
            break;
        default:
            break;
    }

    return exchange_name;
}

}


