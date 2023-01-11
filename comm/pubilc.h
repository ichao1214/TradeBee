//
// Created by haichao.zhang on 2022/1/30.
//

#ifndef TRADEBEE_PUBILC_H
#define TRADEBEE_PUBILC_H



#include <vector>
#include <fstream>
#include <iterator>
#include <iostream>
#include <algorithm>

void split(const std::string & in, char token, std::vector<std::string> & out);

template<class T>
void print_container(std::vector<T> words)
{
    std::ostream_iterator<T> out_iter2 {std::cout, " "};
    std::copy(std::begin(words), std::end(words), out_iter2);
//    out_iter2 = "\n";
}


void get_nowtime(char* output_time);

template <class T>
void binfile2buf(const std::string& bin_file_fullpath, T*& buf,int &total_size)
{
    int per_size = sizeof (T);
    std::ifstream bin_file{bin_file_fullpath, std::ios::binary};
    if (!bin_file.good())
    {
        printf("%s file err\n", std::string(bin_file_fullpath).c_str());
        return;
    }
    bin_file.seekg(0, std::ios::end);    // go to the end

    int length = bin_file.tellg();           // report location (this is the length)
    total_size= length / per_size;// inst_info
    bin_file.seekg(0, std::ios::beg);    // go back to the beginning

    buf = (T*)new char[length];

    bin_file.read((char*)buf, length);                   // read the whole file into the buffer

    bin_file.close();                    // close file handle

}

bool IsValid(double d);

bool IsValid(int i);


int TimeString2Int(const char * time_str);

void GenMinOneDay(int start, int end, std::vector<int> & out);

void GenMin(int start, int end, std::vector<int>& out);

int Adj2Min(int time_);

class DecomposeInteger
{
public:
    int raw_;
    int count_;

    std::vector<std::vector<int>> result;
    std::vector<int> record;

    DecomposeInteger(int raw, int count):raw_(raw),count_(count)
    {

    }

    void Decompose()
    {
        DecomposeFunc(count_, raw_, 0);
        std::transform(result.begin(),result.end(),
                       result.begin(), [](std::vector<int>& r)  {std::reverse(r.begin(),r.end()); return r;});
    }

    void DecomposeFunc(int n, int m, int min)  // 将m拆分成n个数的和，最小元素为min
    {
        static int index = 0;
        if (n == 2)
        {
            for (int i = min+1;i < m/2; ++i)
            {
                record.push_back(i);
                record.push_back(m - i);
                result.push_back(record);
                //printf("case: %d\n", ++index);
                //print();
                record.pop_back();
                record.pop_back();
            }
            return;
        }
        for (int begin = min+1; begin < m /n ; ++begin) {
            record.push_back(begin);
            DecomposeFunc(n-1, m-begin, begin);
            record.pop_back();

        }
    }

    void print()
    {
        for (auto r : record)
            printf("%d\t", r);
        printf("\n======================\n");
    }



};

#endif //TRADEBEE_PUBILC_H