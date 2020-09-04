
//c/c++
#include<stdlib.h>
//linux

//user define
#include"timer.h"

using namespace WSMQ;

int BaseTimer::Begain()
{
    gettimeofday(&m_tBegainTime,NULL);
    return SUCCESS;
}

int ConnectSrvTimer::EpollDown()
{
    gettimeofday(&m_tEpollDoneTime,NULL);
    m_iEpollTime=((m_tEpollDoneTime.tv_sec-m_tBegainTime.tv_sec)*1000000+(m_tEpollDoneTime.tv_usec-m_tBegainTime.tv_usec))/1000;
    return SUCCESS;
}

int ConnectSrvTimer::QueueDataDown()
{
    gettimeofday(&m_tShmQueueDataDoneTime,NULL);
    m_iQueueDataTime=((m_tShmQueueDataDoneTime.tv_sec-m_tEpollDoneTime.tv_sec)*1000000+(m_tShmQueueDataDoneTime.tv_usec-m_tEpollDoneTime.tv_usec))/1000;
    return SUCCESS;
}

int ConnectSrvTimer::GetMaxTimeForQueueData()
{
    m_iMaxTimeForQueueData=100-m_iEpollTime;
    if(m_iMaxTimeForQueueData<50)
    {
        m_iMaxTimeForQueueData=100;
    }
    return m_iMaxTimeForQueueData;
}

bool ConnectSrvTimer::HaveTimeForQueueData()
{
    struct timeval now;
    gettimeofday(&now,NULL);
    return ((now.tv_sec-m_tEpollDoneTime.tv_sec)*1000000+(now.tv_usec-m_tEpollDoneTime.tv_usec))/1000<m_iMaxTimeForQueueData;
}

int LogicSrvTimer::PushMessageDown()
{
    gettimeofday(&m_tPushDoneTime,NULL);
    m_iPushTime=((m_tPushDoneTime.tv_sec-m_tBegainTime.tv_sec)*1000000+(m_tPushDoneTime.tv_usec-m_tBegainTime.tv_usec))/1000;
    return SUCCESS;
}

int LogicSrvTimer::QueueDataDown()
{
    gettimeofday(&m_tShmQueueDataDoneTime,NULL);
    m_iQueueDataTime=((m_tShmQueueDataDoneTime.tv_sec-m_tPushDoneTime.tv_sec)*1000000+(m_tShmQueueDataDoneTime.tv_usec-m_tPushDoneTime.tv_usec))/1000;
    return SUCCESS;
}

int LogicSrvTimer::GetMaxTimeForQueueData()
{
    m_iMaxTimeForQueueData=100-m_iPushTime;
    if(m_iMaxTimeForQueueData<50)
    {
        m_iMaxTimeForQueueData=100;
    }
    return m_iMaxTimeForQueueData;
}

bool LogicSrvTimer::HaveTimeForQueueData()
{
    struct timeval now;
    gettimeofday(&now,NULL);
    return ((now.tv_sec-m_tPushDoneTime.tv_sec)*1000000+(now.tv_usec-m_tPushDoneTime.tv_usec))/1000<m_iMaxTimeForQueueData;
}