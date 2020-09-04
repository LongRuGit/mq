/*
 * @name: 
 * @test: test font
 * @msg: 
 * @param: 
 * @return: 
 */ 
/**
 * @Descripttion: 定义客户端链接对象
 * @Author: readywang
 * @Date: 2020-07-19 14:29:36
 */
#ifndef INCLUDE_CLIENT_CONNECT_H
#define INCLUDE_CLIENT_CONNECT_H

#include<time.h>
#include<list>
#include<unordered_map>
#include<string>

#include"../common_def/comdef.h"

using std::list;
using std::unordered_map;
using std::string;

namespace WSMQ
{
    typedef struct 
    {
        //连接的套接字
        int m_iSockfd;
        //客户端ip
        unsigned int m_iClientAddr;
        //上次收到数据时间
        time_t m_tLastRecvTime;
        //连接是否被使用
        bool m_bUsed;
        //连接是否被放入map
        bool m_bInMap;
        //在保存连接数组中的下标
        int m_iIndex;
        //接收缓冲区
        char *m_pRecvBuff;
        //接收缓冲区尾部
        char *m_pRecvEnd;
        //接收到的数据头部
        char *m_pRecvHead;
        //接收到的数据尾部
        char *m_pRecvTail;

         //发送缓冲区
        char *m_pSendBuff;
        //发送缓冲区尾部
        char *m_pSendEnd;
        //发送的数据头部
        char *m_pSendHead;
        //发送的数据尾部
        char *m_pSendTail;
    }ClientConnect;

    //管理所有的客户端连接，相当于一个连接池
    class ClientConnectManager
    {
    public:
        unsigned long MIN_POINTER_ADDRESS;//所有连接的起始地址
	    unsigned long MAX_POINTER_ADDRESS;//所有连接的终止地址

        const static int SUCCESS=0;   
        const static int ERROR=-1;
    public:
        ClientConnectManager();
        ~ClientConnectManager();
        //判断地址是否有效
        bool IsAddrValid(ClientConnect *ipClientConnect);
        //获取一个可用连接
        ClientConnect *GetOneFreeConnect();
        //客户端退出函数
        void ClientExit(ClientConnect *ipClientConnect,int iEpollfd);
        //根据下标查找用户
        ClientConnect *FindClient(int iIndex);
        //添加在线用户
        int AddOnlineClient(ClientConnect *ipClientConnect);
    private:
        //在线用户
        unordered_map<int,ClientConnect *>m_mOnlineClient;
        //可用连接
        list<ClientConnect *>m_lFreeList;
        //所有连接
        ClientConnect *m_pAllClientConnect;
    };
}

#endif //INCLUDE_CLIENT_CONNECT_H