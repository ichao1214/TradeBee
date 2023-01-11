//
// Created by haichao.zhang on 2022/1/31.
//

#include <string>
#include <vector>

#include <sstream>
#include <iterator>
#include <iostream>
#include <fstream>
#include <chrono>
#include "pubilc.h"


const double EXTREME_SMALL_VALUE = 1E-18;
const double EXTREME_LARGE_VALUE = 1E18;


void split(const std::string& in, char token, std::vector<std::string> & out)
{
    std::istringstream iss(in);
    std::string out_tmp;
    while (getline(iss, out_tmp, token))
        out.push_back(out_tmp);
}



void get_nowtime(char* output_time)
{
    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    auto milliseconds = time % 1000;
    time = time / 1000;
    auto seconds = time % 60;
    time = time / 60;
    auto minutes = time % 60;
    time = time / 60;
    auto hours = (time % 24 + 8) % 24;
    sprintf(output_time, "%02lld:%02lld:%02lld.%03lld", hours, minutes, seconds, milliseconds);
}


bool IsValid(double d)
{
    return d < EXTREME_LARGE_VALUE && d > EXTREME_SMALL_VALUE;
}

bool IsValid(int i)
{
    return i < INT32_MAX && i > INT32_MIN;
}

int TimeString2Int(const char * time_str)
{
    int time_ = 0;
    while (*time_str != '\0')
    {
        if(*time_str >='0' && *time_str <='9')
        {
            int tmp = *time_str - '0';
            time_ = time_ * 10 + tmp;
        }
        time_str++;
    }
    return time_;
}

void GenMinOneDay(int start, int end, std::vector<int> & out)
{
    int interval = 60000;
    for (int i = start; i < end; i = i + interval) {

        int n = i % 100000;
        int m = i/ 100000;
        if (n == 60000)
        {
            i = m * 100000 + 100000;
        }

        int s = i % 10000000;
        int t = i/ 10000000;
        if (s == 6000000)
        {
            i = t * 10000000 + 10000000;
        }

        if(i == end)
        {
            return;
        }
        //printf("%d\n", i);
        out.push_back(i);
    }
}

void GenMin(int start, int end, std::vector<int>& out)
{
    int end1 = -1;
    int start1 = -1;
    if(end < start)
    {
        end1 = end;
        start1 = 0;
        end = 240000000;
    }
    GenMinOneDay(start, end, out);
    if(end1 != -1 && start1 != -1)
        GenMinOneDay(start1, end1, out);
}

int Adj2Min(int time_)
{
    return time_ / 100000 * 100;
}

