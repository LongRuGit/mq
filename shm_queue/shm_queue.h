/*
 * @name: 
 * @test: test font
 * @msg: 
 * @param: 
 * @return: 
 */ 
/**
 * @Descripttion: 实现基于共享内存的队列，便于进程间通信和数据持久化
 * @Author: readywang
 * @Date: 2020-07-18 11:18:28
 */

#ifndef INCLUDE_SEM_QUEUE_H
#define INCLUDE_SEM_QUEUE_H

#include"../sem_lock/sem_lock.h"

#define CAS32(ptr, val_old, val_new)({ char ret; __asm__ __volatile__("lock; cmpxchgl %2,%0; setz %1": "+m"(*ptr), "=q"(ret): "r"(val_new),"a"(val_old): "memory"); ret;})
#define wmb() __asm__ __volatile__("sfence":::"memory")
#define rmb() __asm__ __volatile__("lfence":::"memory")

namespace WSMQ
{   
    class ShmQueue
    {
    public:
        //错误信息定义
        const static int SUCCESS=0;   
        const static int ERROR=-1;
        const static int ERR_SHM_QUEUE_INIT_LOCK=-201;
        const static int ERR_SHM_QUEUE_INIT_SHM=-202;
        const static int ERR_SHM_QUEUE_OPEN_SHM=-203;
        const static int ERR_SHM_QUEUE_AT_SHM=-204;
        const static int ERR_SHM_QUEUE_FULL=-205;
        const static int ERR_SHM_QUEUE_EMPTY=-206;
        const static int ERR_SHM_QUEUE_BUF_SMALL=-207;
        const static int ERR_SHM_QUEUE_DATE_RESET=-208;
        const static int ERR_SHM_QUEUE_DATE_TAIL=-208;

    public:
        //队列头，包括：队列长度，头尾数据块位置及数据块个数,使用者数目
        typedef struct queue_head_s
        {
            int m_iLen;
            int m_iHead;
            int m_iTail;
            int m_iBlockNum;
            int m_iUsedNum;
        }QueueHead;

        //数据块头，包括：当前块起始位置及数据长度
        typedef struct date_block_head_s
        {
            int m_iIndex;
            int m_iDateLen;
        }DateBlockHead;

        //每个数据块末尾都用6个末尾字符（0xDD)标记
        const static char TAIL_FLAG=0xDD;
    public:
        ShmQueue();
        ~ShmQueue();
        //初始化队列
        int Init(int iShmKey,int iQueueSize);

        //数据入队
        int Enqueue(const char *ipDate,int iDateLen);

        //数据出队
        int Dequeue(char *opBuf,int *iopBufLen);

        //获取数据块个数
        int GetDateBlockNum();
        QueueHead *GetQueueHead(){return m_pQueueHead;}
        const char* GetErrMsg(){return m_pErrMsg;}
    private:
        QueueHead *m_pQueueHead; //队列头
        int m_iQueueSize; //队列大小
        char *m_pMemAddr; //内存地址
        SemLock *m_pSemLock; //互斥锁
        char m_pErrMsg[256];
    };
}
#endif //INCLUDE_SEM_QUEUE_H