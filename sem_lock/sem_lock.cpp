//c/c++
#include<string.h>
#include<stdio.h>
//linux
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<errno.h>
#include<time.h>
//user define
#include "sem_lock.h"

using namespace WSMQ;

union semun
{
    int val;
    struct semid_ds*buf;
    unsigned short *array;
};


SemLock::SemLock()
{
    memset(m_pErrMsg,0,sizeof(m_pErrMsg));
    m_iSemKey=0;
    m_iSemId=0;
}

SemLock::~SemLock()
{
}


int SemLock::Init(int iSemKey)
{
    m_iSemKey=iSemKey;
    m_iSemId=semget(m_iSemKey,1,IPC_CREAT|0666);
    if(m_iSemId==-1)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Create semaphore failed! key is %d,errmsg is %s",m_iSemKey,strerror(errno));
        return ERR_SEM_LOCK_INIT;
    }
    semun arg;
    semid_ds semDs;
    arg.buf=&semDs;
    int ret=semctl(m_iSemId,0,IPC_STAT,arg);
    if(ret==-1)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Get semaphore state failed! key is %d,semaphore index is %d,errmsg is %s",m_iSemKey,0,strerror(errno));
        return ERR_SEM_LOCK_INIT;
    }
    //未曾使用或者上次op操作超过3分钟则释放锁
    if(semDs.sem_otime==0||((semDs.sem_otime>0)&&(time(NULL)-semDs.sem_otime>3*60)))
    {
        semun arg;
        arg.val=1;
        ret=semctl(m_iSemId,0,SETVAL,arg);
        if(ret==-1)
        {
            snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Set semaphore val faild! key is %d,semaphore index is %d,errmsg is %s",m_iSemKey,0,strerror(errno));
            return ERR_SEM_LOCK_INIT;
        }
    }
    return SUCCESS;
}

int SemLock::Lock()
{
    if(m_iSemId==0)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Semaphore uninitialized!");
        return ERR_SEM_LOCK_LOCK;
    }
    struct sembuf arg;
    arg.sem_num=0;
    arg.sem_op=-1;
    arg.sem_flg=SEM_UNDO;
    while(1)
    {
        int ret=semop(m_iSemId,&arg,1);
        if(ret==-1)
        {
            if(errno==EINTR)
            {
                continue;
            }
            snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Acquire semaphore faild! key is %d,semaphore index is %d,errmsg is %s",m_iSemKey,0,strerror(errno));
            return ERR_SEM_LOCK_LOCK;
        }
        else
        {
            break;
        }
        
    }
    return SUCCESS;
}

int SemLock::UnLock()
{
    if(m_iSemId==0)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Semaphore uninitialized!");
        return ERR_SEM_LOCK_LOCK;
    }
    struct sembuf arg;
    arg.sem_num=0;
    arg.sem_op=1;
    arg.sem_flg=SEM_UNDO;
    while(1)
    {
        int ret=semop(m_iSemId,&arg,1);
        if(ret==-1)
        {
            if(errno==EINTR)
            {
                continue;
            }
            snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Release semaphore faild! key is %d,semaphore index is %d,errmsg is %s",m_iSemKey,0,strerror(errno));
            return ERR_SEM_LOCK_LOCK;
        }
        else
        {
            break;
        }
        
    }
    return SUCCESS;
}