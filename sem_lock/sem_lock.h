/*
 * @name: 
 * @test: test font
 * @msg: 
 * @param: 
 * @return: 
 */ 
/**
 * @Descripttion: 通过信号量实现互斥锁，便于加锁解锁以及初始化
 * @Author: readywang
 * @Date: 2020-07-18 10:56:34
 */

#ifndef INCLUDE_SEM_LOCK_H
#define INCLUDE_SEM_LOCK_H

namespace WSMQ
{
    class SemLock
    { 
    public:     
        const static int SUCCESS=0;   
        const static int ERROR=-1;
        const static int ERR_SEM_LOCK_INIT=-100;
        const static int ERR_SEM_LOCK_LOCK=-101;
        const static int ERR_SEM_LOCK_UNLOCK=-102; 
    public:
        SemLock();
        ~SemLock();
        int Init(int iSemKey);
        int Lock();
        int UnLock();
        const char *GetErrMsg()
        {
            return m_pErrMsg;
        }
    private:
        int m_iSemKey;
        int m_iSemId;
        char m_pErrMsg[256];
    };
}
#endif //INCLUDE_SEM_LOCK_H