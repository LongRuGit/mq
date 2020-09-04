/*
 * @name: 
 * @test: test font
 * @msg: 
 * @param: 
 * @return: 
 */ 
/**
 * @Descripttion: 定时控制容器
 * @Author: readywang
 * @Date: 2020-07-19 15:27:12
 */
#ifndef INCLUDE_TIMER_H
#define INCLUDE_TIMER_H

#include<sys/time.h>

namespace WSMQ
{
    class BaseTimer
    {
    public:
        const static int SUCCESS=0;   
        const static int ERROR=-1;
    public:
        //记录开始处理时间
        int Begain();
        virtual int QueueDataDown() = 0;
        virtual int GetMaxTimeForQueueData()=0;
        virtual bool HaveTimeForQueueData()=0;
    protected:
        struct timeval m_tBegainTime;//开始时间
        struct timeval m_tShmQueueDataDoneTime; //处理共享内存队列数据结束时间
        int m_iQueueDataTime; //处理队列数据花费时间
        int m_iMaxTimeForQueueData; //处理队列数据所需要的最大时间
    };
    class ConnectSrvTimer:public BaseTimer
    {
    public:
        //记录epoll结束时间，并计数epoll花费时间
        int EpollDown();
        //计算读取共享内存队列数据结束时间，并计算读取花费时间
        int QueueDataDown();
        //返回处理完epoll数据后还有多长时间供于处理队列数据
        int GetMaxTimeForQueueData();
        //是否还有时间处理队列数据
        bool HaveTimeForQueueData();
    protected:
        struct timeval m_tEpollDoneTime; //处理完epoll数据的时间
        int m_iEpollTime; //处理epoll数据花费时间
    };
    class LogicSrvTimer:public BaseTimer
    {
    public:
        //记录将消息队列中现有消息推送出去时间
        int PushMessageDown();
        //计算读取共享内存队列数据结束时间，并计算读取花费时间
        int QueueDataDown();
        //返回推送完消息后还有多长时间供于处理共享内存队列数据
        int GetMaxTimeForQueueData();
        //是否还有时间处理共享内存队列队列数据
        bool HaveTimeForQueueData();
    protected:
        struct timeval m_tPushDoneTime; //推送完消息的时间
        int m_iPushTime; //推送消息花费时间
    };
}

#endif //INCLUDE_TIMER_H