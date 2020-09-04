/*
 * @name: 
 * @test: test font
 * @msg: 
 * @param: 
 * @return: 
 */ 
/**
 * @Descripttion: 接入层，负责和客户端通信，和业务逻辑层交互
 * @Author: readywang
 * @Date: 2020-07-19 10:26:27
 */

#ifndef INCLUDE_CONNECT_SEVER_H
#define INCLUDE_CONNECT_SEVER_H

#include <stdio.h>
#include<sys/epoll.h>
#include"clientConnect.h"
#include"../shm_queue/shm_queue.h"

namespace WSMQ
{
    class ConnectServer
    {
    public:
        const static int SUCCESS=0;   
        const static int ERROR=-1;
        const static int ERR_CONNECT_SERVER_INIT=-401;
        const static int ERR_CONNECT_SERVER_RUN=-402;
        const static int ERR_CONNECT_SERVER_GET_PACKAGE=-403;
        const static int ERR_CONNECT_SERVER_SEND_PACKAGE=-404;
        
    public:
        static ConnectServer *GetInstance();
        static void Destroy();
        int Init();
        int Run();
    protected:
        int InitConf(const char *ipPath);
        int ProcessEpollData(struct epoll_event *ipEvents,int iNum);
        int ProcessShmQueueData();
        int GetClientPackageFromBuff(char *ipBuff,int ipBuffLen,int iClientIndex,char **opNextPack,int *opPackLen);
        int SendDataToClient(const char *ipData,int iDataLen,ClientConnect *ipClient);
        int InitSigHandler();
        ConnectServer();
        ~ConnectServer();
        static void SigTermHandler(int iSig){ m_bStop = true; }
    protected:
        int m_iEpollFd;
        int m_iListenFd;
        ClientConnectManager *m_pClientConnectManager;
        ShmQueue *m_pQueueToLogicLayer;
        ShmQueue *m_pQueueLogicToConnect;
        static bool m_bStop;
        static ConnectServer *m_pConnectServer;
        int m_iSendCount;
        int m_iRecvCount;
    };
    ConnectServer *ConnectServer::m_pConnectServer=NULL;
    bool ConnectServer::m_bStop=true;
}
#endif //INCLUDE_CONNECT_SEVER_H