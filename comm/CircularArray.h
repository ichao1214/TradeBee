//
// Created by haichao.zhang on 2022/1/9.
//

#ifndef TRADEBEE_CIRCULARARRAY_H
#define TRADEBEE_CIRCULARARRAY_H

#include <mutex>
#include <condition_variable>


class XSemaphore{
public:
    XSemaphore(int nCount=0):m_count(nCount){}

    void Wait(){
        std::unique_lock<std::mutex> lker(m_mtx);
        m_cv.wait(lker, [this](){return m_count>0;});
        --m_count;
    }

    void Signal(){
        {
            std::unique_lock<std::mutex> lker(m_mtx);
            ++m_count;
        } // 通知前解锁，避免获取信号后因得不到锁重新进入等待
        m_cv.notify_one();
    }

private:
    int m_count;
    std::mutex m_mtx;
    std::condition_variable m_cv;
};

template<typename T>
class CircularArray {
public:
    explicit CircularArray(const size_t elems) {
        cap = elems;
        push_index_ = 0;
        pop_index_ = 0;
        arr.resize(cap);
        recv_semaphore = std::make_shared<XSemaphore>();
        left_space_semaphore = std::make_shared<XSemaphore>(cap);

    };

    void push(const std::shared_ptr<T>& data)
    {
        left_space_semaphore->Wait();
        //printf("push index %d, real pos %d\n", push_index_, push_index_ % cap);
        arr[ push_index_++  % cap] = std::move(data);
        recv_semaphore->Signal();
    }

    std::shared_ptr<T> pop()
    {
        recv_semaphore->Wait();
        //printf("pop index %d, real pos %d\n", pop_index_, pop_index_ % cap);
        auto elem = arr[pop_index_++ % cap];
        left_space_semaphore->Signal();
        return elem;
    }

    size_t getCapacity()
    {
        return cap;
    }

    ~CircularArray()
    {
       printf("~CircularArray dtor\n");
    }
private:
    std::vector<std::shared_ptr<T>> arr ;

    size_t cap;
    size_t push_index_;
    size_t pop_index_;

    std::shared_ptr<XSemaphore> recv_semaphore;
    std::shared_ptr<XSemaphore> left_space_semaphore;
};


#endif //TRADEBEE_CIRCULARARRAY_H
