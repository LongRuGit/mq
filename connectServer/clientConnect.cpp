//c/c++
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
//linux
#include<sys/epoll.h>
#include<unistd.h>
//user define
#include"clientConnect.h"

using namespace WSMQ;

ClientConnectManager::ClientConnectManager()
{
    m_pAllClientConnect=new ClientConnect[CLIENT_EPOLL_COUNT];
    if(!m_pAllClientConnect)
    {
        //写日志
    }
    else
    {
        for(int i=0;i<CLIENT_EPOLL_COUNT;++i)
        {
            m_pAllClientConnect[i].m_bUsed=false;
            m_pAllClientConnect[i].m_bInMap=false;
            m_pAllClientConnect[i].m_iIndex=i;
            m_pAllClientConnect[i].m_pRecvBuff=(char *)malloc(SERVER_BUFFER_SIZE);
            memset(m_pAllClientConnect[i].m_pRecvBuff,0,SERVER_BUFFER_SIZE);
            m_pAllClientConnect[i].m_pRecvHead=m_pAllClientConnect[i].m_pRecvBuff;
            m_pAllClientConnect[i].m_pRecvTail=m_pAllClientConnect[i].m_pRecvBuff;
            m_pAllClientConnect[i].m_pRecvEnd=m_pAllClientConnect[i].m_pRecvBuff+SERVER_BUFFER_SIZE;

            m_pAllClientConnect[i].m_pSendBuff=(char *)malloc(SERVER_BUFFER_SIZE);
            memset(m_pAllClientConnect[i].m_pSendBuff,0,SERVER_BUFFER_SIZE);
            m_pAllClientConnect[i].m_pSendHead=m_pAllClientConnect[i].m_pSendBuff;
            m_pAllClientConnect[i].m_pSendTail=m_pAllClientConnect[i].m_pSendBuff;
            m_pAllClientConnect[i].m_pSendEnd=m_pAllClientConnect[i].m_pSendBuff+SERVER_BUFFER_SIZE;
            m_lFreeList.push_back(&m_pAllClientConnect[i]);
        }
        MIN_POINTER_ADDRESS=(unsigned long)m_pAllClientConnect;
        MAX_POINTER_ADDRESS=(unsigned long)(m_pAllClientConnect+CLIENT_EPOLL_COUNT);
    }
}

ClientConnectManager::~ClientConnectManager()
{
    if(m_pAllClientConnect)
    {
        for(int i=0;i<CLIENT_EPOLL_COUNT;++i)
        {
            free(m_pAllClientConnect[i].m_pRecvBuff);
        }
        delete []m_pAllClientConnect;
        m_pAllClientConnect=NULL;
    }
}

bool ClientConnectManager::IsAddrValid(ClientConnect *ipClientConnect)
{
    return (unsigned long)ipClientConnect>=MIN_POINTER_ADDRESS&&(unsigned long)ipClientConnect<=MAX_POINTER_ADDRESS
    &&((unsigned long)ipClientConnect-MIN_POINTER_ADDRESS)%sizeof(ClientConnect)==0;
}

ClientConnect *ClientConnectManager::GetOneFreeConnect()
{
    if(m_lFreeList.empty())
    {
        return NULL;
    }
    ClientConnect *pFront=m_lFreeList.front();
    m_lFreeList.pop_front();
    if(!IsAddrValid(pFront))
    {
        return NULL;
    }
    pFront->m_bUsed=true;
    return pFront;
}

void ClientConnectManager::ClientExit(ClientConnect *ipClientConnect,int iEpollfd)
{
    if(ipClientConnect==NULL)
    {
        return;
    }
    if(!ipClientConnect->m_bUsed)
    {
        return;
    }
    if(ipClientConnect->m_iSockfd==0)
    {
        //日志
    }
    else
    {
        close(ipClientConnect->m_iSockfd);
        int ret=epoll_ctl(iEpollfd,EPOLL_CTL_DEL,ipClientConnect->m_iSockfd,NULL);
        if(ret<0)
        {
            epoll_event event = { 0, { 0 } };
			event.events = 0;
			//ev.data.fd = pPetClient->iSockfd;
			event.data.ptr = ipClientConnect;
			ret = epoll_ctl(iEpollfd, EPOLL_CTL_MOD, ipClientConnect->m_iSockfd, &event);
			if(0 != ret)
			{
				//日志
			}
			else
			{
				//日志
			}

        }
    }
    //将其从hashmap移除，放入空闲链表
    if(m_mOnlineClient.find(ipClientConnect->m_iIndex)!=m_mOnlineClient.end())
    {
        m_mOnlineClient.erase(ipClientConnect->m_iIndex);
    }
    //清空原先缓冲区数据并重置
    memset(ipClientConnect->m_pRecvBuff,0,SERVER_BUFFER_SIZE);
    ipClientConnect->m_pRecvHead=ipClientConnect->m_pRecvBuff;
    ipClientConnect->m_pRecvTail=ipClientConnect->m_pRecvBuff;
    memset(ipClientConnect->m_pSendBuff,0,SERVER_BUFFER_SIZE);
    ipClientConnect->m_pRecvHead=ipClientConnect->m_pSendBuff;
    ipClientConnect->m_pRecvTail=ipClientConnect->m_pSendBuff;
    m_lFreeList.push_back(ipClientConnect);
    ipClientConnect->m_bInMap=false;
    ipClientConnect->m_bUsed=false;
}

ClientConnect *ClientConnectManager::FindClient(int iIndex)
{
    //从在线用户中查找
    unordered_map<int,ClientConnect *>::iterator it=m_mOnlineClient.find(iIndex);
    if(it!=m_mOnlineClient.end())
    {
        return it->second;
    }
    else
    {
        return NULL;
    }
    
}

int ClientConnectManager::AddOnlineClient(ClientConnect *ipClientConnect)
{
    if(ipClientConnect==NULL)
    {
        return ERROR;
    }
    m_mOnlineClient[ipClientConnect->m_iIndex]=ipClientConnect;
    return SUCCESS;
}