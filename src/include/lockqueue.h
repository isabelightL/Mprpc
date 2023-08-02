#pragma once
#include<queue>
#include<thread>
#include<mutex> //相当于linux下的 pthread_mutex_t-------线程互斥，互斥锁
#include<condition_variable> // pthread_condition_t------线程间通信，条件变量

//异步写日志的日志队列
template<typename T>
class LockQueue
{
private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condvariable;
public:
    //多个worker线程都会写日志queue
    void Push(const T &data)
    {
        std::lock_guard<std::mutex> lock(m_mutex);//使用了智能指针lock_guard<std::mutex>，自动 加锁释放锁
        m_queue.push(data);
        m_condvariable.notify_one();//因为只有一个线程从队列里拿数据，所以不用notify_all()

    }
    //一个线程读日志queue,写日志文件
    T Pop()
    {
        std::unique_lock<std::mutex>lock(m_mutex);//条件变量需要的，unique_lock
        while(m_queue.empty())
        {
            //日志队列为空，线程进入wait状态，并把所持有锁释放了（入队和出队用的是同一把锁）
            m_condvariable.wait(lock);
        }

        T data =m_queue.front();
        m_queue.pop();
        return data;

        
    }
};


