//c/c++
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
//linux
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<poll.h>
//user define
#include"client.h"
#include"../mq_util/mq_util.h"
#include"../logger/logger.h"

using namespace WSMQ;


//全局变量 
Logger clientLogger;

Client::Client()
{
    m_iSockfd=-1;
    m_bBuild=false;
    m_iConfirmLevel=0;
    memset(m_pErrMsg,0,sizeof(m_pErrMsg));
    m_pRecvBuff=(char *)malloc(SOCK_RECV_BUFF_SIZE);
    memset(m_pRecvBuff,0,SOCK_RECV_BUFF_SIZE);
    m_pRecvHead=m_pRecvBuff;
    m_pRecvTail=m_pRecvBuff;
    m_pRecvEnd=m_pRecvBuff+SOCK_RECV_BUFF_SIZE;

    m_pSendBuff=(char *)malloc(SOCK_SEND_BUFF_SIZE);
    memset(m_pSendBuff,0,SOCK_SEND_BUFF_SIZE);
    m_pSendHead=m_pSendBuff;
    m_pSendTail=m_pSendBuff;
    m_pSendEnd=m_pSendBuff+SOCK_SEND_BUFF_SIZE;
}

Client::~Client()
{
    if(m_iSockfd>0)
    {
        close(m_iSockfd);
    }
    if(m_pRecvBuff)
    {
        free(m_pRecvBuff);
        m_pRecvBuff=NULL;
    }
    if(m_pSendBuff)
    {
        free(m_pSendBuff);
        m_pSendBuff=NULL;
    }
}

int Client::BuildConnection(const char *ipSeverIp/*=SERVER_DEFAULT_IP_ADDR*/,unsigned short iServerPort/*=SERVER_DEFAULT_PORT*/)
{
    m_iSockfd=socket(AF_INET,SOCK_STREAM,0);
    if(m_iSockfd<=0)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Create Sockfd failed!");
        return ERR_CLIENT_BUILD_CONNECT;
    }
    sockaddr_in serverAddr;
    memset(&serverAddr,0,sizeof(serverAddr));
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_port=htons(iServerPort);
    serverAddr.sin_addr.s_addr=inet_addr(ipSeverIp);
    int ret=connect(m_iSockfd,(struct sockaddr *)&serverAddr,sizeof(serverAddr));
    if(ret==-1)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"connect failed,ip is %s,port is %d!,err msg is %s",ipSeverIp,iServerPort,strerror(errno));
        return ERR_CLIENT_BUILD_CONNECT;
    }
    FuncTool::SetNonBlock(m_iSockfd);
    int iSendBufSize=SOCK_SEND_BUFF_SIZE;
    int iRevBufSize=SOCK_RECV_BUFF_SIZE;
    setsockopt(m_iSockfd,SOL_SOCKET,SO_SNDBUF,(char *)&iSendBufSize,sizeof(int));
    setsockopt(m_iSockfd,SOL_SOCKET,SO_RCVBUF,(char *)&iRevBufSize,sizeof(int));
    m_bBuild=true;
    return SUCCESS;
}

int Client::CreateExchange(const string &istrName,unsigned short iExchangeType,bool ibDurable/*=false*/,bool ibAutoDel/*=true*/)
{
    if(m_bBuild==false)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection not build!");
        return ERR_CLIENT_BUILD_CONNECT;
    }
    CreateExchangeMessage oMsg(istrName,iExchangeType,ibDurable,ibAutoDel);
    //获取消息包
    char pMsgPack[MAX_CLINT_PACKAGE_LENGTH];
    int iPackLen=MAX_CLINT_PACKAGE_LENGTH;
    memset(pMsgPack,0,sizeof(pMsgPack));
    int ret=0;
    if((ret=oMsg.GetMessagePack(pMsgPack,&iPackLen))!=Message::SUCCESS)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Invaild parms!");
        return ERR_CLIENT_CREATE_EXCHANGE;
    }
    //发送数据
    ret=SendOnePack(pMsgPack,iPackLen);
    if(ret!=Client::SUCCESS)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection closed!");
        close(m_iSockfd);
        m_bBuild=false;
        return ERR_CLIENT_CREATE_EXCHANGE;
    }
    //接收回复
    char pBuffer[MAX_SERVER_PACKAGE_LENGTH];
    int iLen=MAX_SERVER_PACKAGE_LENGTH;
    memset(pBuffer,0,sizeof(pBuffer));
    unsigned short iReplyType=0;
    if((ret=RecvOnePack(pBuffer,&iLen,iReplyType))!=Client::SUCCESS||iReplyType!=CMD_SERVER_REPLY_MESSAGE)
    {
        return ERR_CLIENT_CREATE_EXCHANGE;
    }
    //解析回复包
    char *pTemp=pBuffer;
    bool bSucceed=true;
    int offset=FuncTool::ReadBool(pTemp,bSucceed);
    if(!bSucceed)
    {
        pTemp+=offset;
        offset=FuncTool::ReadBuf(pTemp,m_pErrMsg,sizeof(m_pErrMsg));
        return ERR_CLIENT_CREATE_EXCHANGE;
    }
    return SUCCESS;
}

int Client::CreateQueue(const string &istrName,short iPriority/*=-1*/,bool ibDurable/*=false*/,bool ibAutoDel/*=true*/)
{
    if(m_bBuild==false)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection not build!");
        return ERR_CLIENT_BUILD_CONNECT;
    }
    CreateQueueMessage oMsg(istrName,iPriority,ibDurable,ibAutoDel);
    //获取消息包
    char pMsgPack[MAX_CLINT_PACKAGE_LENGTH];
    int iPackLen=MAX_CLINT_PACKAGE_LENGTH;
    memset(pMsgPack,0,sizeof(pMsgPack));
    int ret=0;
    if((ret=oMsg.GetMessagePack(pMsgPack,&iPackLen))!=Message::SUCCESS)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Invaild parms!");
        return ERR_CLIENT_CREATE_QUEUE;
    }
    //发送数据
    ret=SendOnePack(pMsgPack,iPackLen);
    if(ret!=Client::SUCCESS)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection closed!");
        close(m_iSockfd);
        m_bBuild=false;
        return ERR_CLIENT_CREATE_QUEUE;
    }

    //接收回复
    char pBuffer[MAX_SERVER_PACKAGE_LENGTH];
    int iLen=MAX_SERVER_PACKAGE_LENGTH;
    memset(pBuffer,0,sizeof(pBuffer));
    unsigned short iReplyType=0;
    if((ret=RecvOnePack(pBuffer,&iLen,iReplyType))!=Client::SUCCESS||iReplyType!=CMD_SERVER_REPLY_MESSAGE)
    {
        return ERR_CLIENT_CREATE_QUEUE;
    }
    //解析回复包
    char *pTemp=pBuffer;
    bool bSucceed=true;
    int offset=FuncTool::ReadBool(pTemp,bSucceed);
    if(!bSucceed)
    {
        pTemp+=offset;
        offset=FuncTool::ReadBuf(pTemp,m_pErrMsg,sizeof(m_pErrMsg));
        return ERR_CLIENT_CREATE_QUEUE;
    }
    return SUCCESS;
}

int Client::CreateBinding(const string &istrExName, const string &istrQueueName, const string &istrKey)
{
    if(m_bBuild==false)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection not build!");
        return ERR_CLIENT_BUILD_CONNECT;
    }
    CreateBindingMessage oMsg(istrExName,istrQueueName,istrKey);
    //获取消息包
    char pMsgPack[MAX_CLINT_PACKAGE_LENGTH];
    int iPackLen=MAX_CLINT_PACKAGE_LENGTH;
    memset(pMsgPack,0,sizeof(pMsgPack));
    int ret=0;
    if((ret=oMsg.GetMessagePack(pMsgPack,&iPackLen))!=Message::SUCCESS)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Invaild parms!");
        return ERR_CLIENT_CREATE_BINDING;
    }
    //发送数据
    ret=SendOnePack(pMsgPack,iPackLen);
    if(ret!=Client::SUCCESS)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection closed!");
        close(m_iSockfd);
        m_bBuild=false;
        return ERR_CLIENT_CREATE_BINDING;
    }

    //接收回复
    char pBuffer[MAX_SERVER_PACKAGE_LENGTH];
    int iLen=MAX_SERVER_PACKAGE_LENGTH;
    memset(pBuffer,0,sizeof(pBuffer));
    unsigned short iReplyType=0;
    if((ret=RecvOnePack(pBuffer,&iLen,iReplyType))!=Client::SUCCESS||iReplyType!=CMD_SERVER_REPLY_MESSAGE)
    {
        return ERR_CLIENT_CREATE_BINDING;
    }
    //解析回复包
    char *pTemp=pBuffer;
    bool bSucceed=true;
    int offset=FuncTool::ReadBool(pTemp,bSucceed);
    if(!bSucceed)
    {
        pTemp+=offset;
        offset=FuncTool::ReadBuf(pTemp,m_pErrMsg,sizeof(m_pErrMsg));
        return ERR_CLIENT_CREATE_BINDING;
    }
    return SUCCESS;
}

int Client::DeleteExchange(const string &istrName)
{
    if(m_bBuild==false)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection not build!");
        return ERR_CLIENT_BUILD_CONNECT;
    }
    DeleteExchangeMessage oMsg(istrName);
    //获取消息包
    char pMsgPack[MAX_CLINT_PACKAGE_LENGTH];
    int iPackLen=MAX_CLINT_PACKAGE_LENGTH;
    memset(pMsgPack,0,sizeof(pMsgPack));
    int ret=0;
    if((ret=oMsg.GetMessagePack(pMsgPack,&iPackLen))!=Message::SUCCESS)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Invaild parms!");
        return ERR_CLIENT_DELETE_EXCHANGE;
    }
    //发送数据
    ret=SendOnePack(pMsgPack,iPackLen);
    if(ret!=Client::SUCCESS)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection closed!");
        close(m_iSockfd);
        m_bBuild=false;
        return ERR_CLIENT_DELETE_EXCHANGE;
    }

    //接收回复
    char pBuffer[MAX_SERVER_PACKAGE_LENGTH];
    int iLen=MAX_SERVER_PACKAGE_LENGTH;
    memset(pBuffer,0,sizeof(pBuffer));
    unsigned short iReplyType=0;
    if((ret=RecvOnePack(pBuffer,&iLen,iReplyType))!=Client::SUCCESS||iReplyType!=CMD_SERVER_REPLY_MESSAGE)
    {
        return ERR_CLIENT_DELETE_EXCHANGE;
    }
    //解析回复包
    char *pTemp=pBuffer;
    bool bSucceed=true;
    int offset=FuncTool::ReadBool(pTemp,bSucceed);
    if(!bSucceed)
    {
        pTemp+=offset;
        offset=FuncTool::ReadBuf(pTemp,m_pErrMsg,sizeof(m_pErrMsg));
        return ERR_CLIENT_DELETE_EXCHANGE;
    }
    return SUCCESS;
}

int Client::DeleteQueue(const string &istrName)
{
    if(m_bBuild==false)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection not build!");
        return ERR_CLIENT_BUILD_CONNECT;
    }
    DeleteQueueMessage oMsg(istrName);
    //获取消息包
    char pMsgPack[MAX_CLINT_PACKAGE_LENGTH];
    int iPackLen=MAX_CLINT_PACKAGE_LENGTH;
    memset(pMsgPack,0,sizeof(pMsgPack));
    int ret=0;
    if((ret=oMsg.GetMessagePack(pMsgPack,&iPackLen))!=Message::SUCCESS)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Invaild parms!");
        return ERR_CLIENT_DELETE_QUEUE;
    }
    //发送数据
    ret=SendOnePack(pMsgPack,iPackLen);
    if(ret!=Client::SUCCESS)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection closed!");
        close(m_iSockfd);
        m_bBuild=false;
        return ERR_CLIENT_DELETE_QUEUE;
    }

    //接收回复
    char pBuffer[MAX_SERVER_PACKAGE_LENGTH];
    int iLen=MAX_SERVER_PACKAGE_LENGTH;
    memset(pBuffer,0,sizeof(pBuffer));
    unsigned short iReplyType=0;
    if((ret=RecvOnePack(pBuffer,&iLen,iReplyType))!=Client::SUCCESS||iReplyType!=CMD_SERVER_REPLY_MESSAGE)
    {
        return ERR_CLIENT_DELETE_QUEUE;
    }
    //解析回复包
    char *pTemp=pBuffer;
    bool bSucceed=true;
    int offset=FuncTool::ReadBool(pTemp,bSucceed);
    if(!bSucceed)
    {
        pTemp+=offset;
        offset=FuncTool::ReadBuf(pTemp,m_pErrMsg,sizeof(m_pErrMsg));
        return ERR_CLIENT_DELETE_QUEUE;
    }
    return SUCCESS;
}

int Client ::WaitServerReply()
{
    struct pollfd fds[1];
    fds[0].fd=m_iSockfd;
    fds[0].events=POLLIN|POLLERR;
    while(true)
    {
        int ret=poll(fds,1,-1);
        if(ret<0)
        {
            if(errno==EINTR||errno==EAGAIN)
            {
                continue;
            }
            return ERROR;
        }
        else if(ret==0)
        {
            return ERROR;
        }
        else if(fds[0].revents&POLLERR)
        {
            return ERROR;
        }
        else
        {
            break;
        }
    }
    return SUCCESS;
}

int Client::SendOnePack(const char *ipBuffer,int iLen)
{
    if(ipBuffer==NULL||iLen<=0||m_iSockfd<=0)
    {
        return ERR_CLIENT_SEND_PACK;
    }
    //等待套接字可写
    struct pollfd fds[1];
    fds[0].fd=m_iSockfd;
    fds[0].events=POLLOUT|POLLERR;
    while(true)
    {
        int ret=poll(fds,1,-1);
        if(ret<0)
        {
            if(errno==EINTR||errno==EAGAIN)
            {
                continue;
            }
            return ERR_CLIENT_SEND_PACK;
        }
        else if(ret==0)
        {
            return ERR_CLIENT_SEND_PACK;
        }
        else if(fds[0].revents&POLLOUT)
        {
            break;
        }
        else
        {
            return ERR_CLIENT_SEND_PACK;
        }
    }
    int ret=FuncTool::Sendn(m_iSockfd,ipBuffer,iLen);
    if(ret==FuncTool::ERROR)
    {
        return ERR_CLIENT_SEND_PACK;
    }
    return SUCCESS;
}

int Client::SendPack()
{
    int iSendBytes=m_pSendTail-m_pSendHead;
    if(iSendBytes>0)
    {
        int iRet=send(m_iSockfd,m_pSendHead,iSendBytes,0);
        if(iRet<0)
        {
            if(ERROR==EWOULDBLOCK)
            {
                //缓冲区满了直接返回
                return ERR_SOCKET_SEND_BUFF_FULL;
            }
            else
            {
                snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection closed!");
                close(m_iSockfd);
                m_bBuild=false;
                return ERR_SEVER_CLOSED;
            }
        }
        else if(iRet==0)
        {
            snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection closed!");
            close(m_iSockfd);
            m_bBuild=false;
            return ERR_SEVER_CLOSED;
        }
        else
        {
            //更新发送缓冲区头部
            m_pSendHead+=iRet;
        }
    }
    return SUCCESS;
}

// int Client:: RecvOnePack(char *opBuffer,int *iopLen,unsigned short &oRelyType)
// {
//     if(m_bBuild==false)
//     {
//         snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection not build!");
//         return ERR_CLIENT_BUILD_CONNECT;
//     }
//      //接收数据包头部
//     int ret=0;
//     ClientPackageHead header;
//     char pBuffer[MAX_SERVER_PACKAGE_LENGTH];
//     memset(pBuffer,0,sizeof(pBuffer));
//     ret=FuncTool::Recvn(m_iSockfd,pBuffer,sizeof(header));
//     if(ret!=FuncTool::SUCCESS)
//     {
//         snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection closed!");
//         m_bBuild=false;
//         close(m_iSockfd);
//         return ERR_CLIENT_REV_PACK;
//     }
    
//     //读取头部信息
//     char *pTemp=pBuffer;
//     int offset=FuncTool::ReadShort(pTemp,header.m_iPackLen);
//     pTemp+=offset;
//     offset=FuncTool::ReadInt(pTemp,header.m_iClientIndex);
//     pTemp+=offset;
//     offset=FuncTool::ReadShort(pTemp,header.m_iCmdId);
//     oRelyType=header.m_iCmdId;
//     pTemp+=offset;
//     if(header.m_iPackLen>MAX_CLINT_PACKAGE_LENGTH-sizeof(ClientPackageHead))
//     {
//         snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Msg is too long!");
//         return ERR_CLIENT_REV_PACK;
//     }
//     int iPackLen=header.m_iPackLen-sizeof(header);
//     //接收数据
//     ret=FuncTool::Recvn(m_iSockfd,pTemp,iPackLen);
//     if(ret!=FuncTool::SUCCESS)
//     {
//         snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection closed!");
//         m_bBuild=false;
//         close(m_iSockfd);
//         return ERR_CLIENT_REV_PACK;
//     }
//     FuncTool::ReadBuf(pTemp,opBuffer,iPackLen);
//     *iopLen=iPackLen;
//     return SUCCESS;
// }

int Client:: RecvOnePack(char *opBuffer,int *iopLen,unsigned short &oRelyType)
{
    if(m_bBuild==false)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection not build!");
        return ERR_CLIENT_BUILD_CONNECT;
    }
    //接收数据包
    int iRet=0;
    do
    {
        iRet=RecvPack();
        if(iRet==ERR_SEVER_CLOSED)
        {
            return ERR_CLIENT_REV_PACK;
        }
    } while (iRet!=SUCCESS);
    
    //读取头部信息
    ClientPackageHead header;
    char *pTemp=m_pRecvHead;
    int offset=FuncTool::ReadShort(pTemp,header.m_iPackLen);
    pTemp+=offset;
    offset=FuncTool::ReadInt(pTemp,header.m_iClientIndex);
    pTemp+=offset;
    offset=FuncTool::ReadShort(pTemp,header.m_iCmdId);
    oRelyType=header.m_iCmdId;
    pTemp+=offset;
    if(header.m_iPackLen>MAX_CLINT_PACKAGE_LENGTH-sizeof(ClientPackageHead))
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Msg is too long!");
        return ERR_CLIENT_REV_PACK;
    }
    int iDataLen=header.m_iPackLen-sizeof(header);
    memcpy(opBuffer, pTemp, iDataLen);
    *iopLen=iDataLen;
    //调整接收数据的起始位置
    m_pRecvHead+=header.m_iPackLen;
    int iMoveCount=m_pRecvTail-m_pRecvHead;
    memmove(m_pRecvBuff,m_pRecvHead,iMoveCount);
    m_pRecvHead=m_pRecvBuff;
    m_pRecvTail=m_pRecvHead+iMoveCount;
    return SUCCESS;
}

int Client::RecvPack()
{
    int iLen=m_pRecvTail-m_pRecvHead;
    unsigned short iPackLen=0;
    bool bNeedRecv=false;//是否需要接收
    if(iLen>sizeof(ClientPackageHead))
    {
        FuncTool::ReadShort(m_pRecvHead,iPackLen);
        if(iLen<iPackLen)
        {
            bNeedRecv=true;
        }
    }
    else
    {
        bNeedRecv=true;
    }
    //若需要接收，则进行接收
    if(bNeedRecv)
    {
        //等待接收
        int ret=WaitServerReply();
        if(ret!=SUCCESS)
        {
            snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection closed 1!");
            close(m_iSockfd);
            m_bBuild=false;
            return ERR_SEVER_CLOSED;
        }
        int iFreeSpace=m_pRecvEnd-m_pRecvTail;
        ret=recv(m_iSockfd,m_pRecvTail,iFreeSpace,0);
        if(ret<0)
        {
            if(errno==EAGAIN)
            {
                return ERROR;
            }
            snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection closed 2!");
            close(m_iSockfd);
            m_bBuild=false;
            return ERR_SEVER_CLOSED;
        }
        else if(ret==0)
        {
            snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection closed 3,free space is %d!",iFreeSpace);
            close(m_iSockfd);
            m_bBuild=false;
            return ERR_SEVER_CLOSED;
        }
        m_pRecvTail+=ret;
        //再次检查
        iLen=m_pRecvTail-m_pRecvHead;
        if(iLen>sizeof(ClientPackageHead))
        {
            FuncTool::ReadShort(m_pRecvHead,iPackLen);
            if(iLen<iPackLen)
            {
                return ERROR;
            }
            else
            {
                return SUCCESS;
            }
            
        }
        else
        {
            return ERROR;
        }
    }
    else
    {
        return SUCCESS;
    }
}

Producer::Producer()
{
    m_iMsgSeq=0;
}

Producer::~Producer()
{
    if(m_pRecvBuff)
    {
        free(m_pRecvBuff);
        m_pRecvBuff=NULL;
    }
}

int Producer:: PuslishMessage(const string &istrExName,const string &istrKey,
const string &istrMsgBody, short iPriority/*=-1*/,bool ibDurable/*=false*/)
{
    if(m_bBuild==false)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection not build!");
        return ERR_CLIENT_BUILD_CONNECT;
    }
    CreatePublishMessage oMsg(istrExName,istrKey,istrMsgBody,iPriority,ibDurable,m_iConfirmLevel);
    //如果需要确认，设置消息序号
    if(m_iConfirmLevel>0)
    {
        oMsg.SetMsgSeq(m_iMsgSeq++);
    }
    
    //获取消息包
    char pMsgPack[MAX_CLINT_PACKAGE_LENGTH];
    memset(pMsgPack,0,MAX_CLINT_PACKAGE_LENGTH);
    int iPackLen=MAX_CLINT_PACKAGE_LENGTH;
    int ret=0;
    if((ret=oMsg.GetMessagePack(pMsgPack,&iPackLen))!=Message::SUCCESS)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Invaild parms!");
        return ERR_CLIENT_PUBLISH_MSG;
    }
    //发送数据
    ret=SendOnePack(pMsgPack,iPackLen);
    if(ret!=Client::SUCCESS)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Server closed!");
        close(m_iSockfd);
        m_bBuild=false;
        return ERR_CLIENT_PUBLISH_MSG;
    }
    //若要确认，则将消息保存
    if(m_iConfirmLevel>0)
    {
        oMsg.SetSaveTime();
        m_NeedAckMsgList.push_back(oMsg);
    }
    return SUCCESS;
}

int Producer::RecvServerAck()
{
    //接收数据包
    int iRet=0;
    do
    {
        iRet=RecvPack();
        if(iRet==ERR_SEVER_CLOSED)
        {
            return ERR_SEVER_CLOSED;
        }
    } while (iRet!=SUCCESS);
    int iLen=m_pRecvTail-m_pRecvHead;
    ClientPackageHead header;
    unordered_set<int>sAckSeq;
    while(true)
    {
        if(iLen<(int)sizeof(header))
        {
            //收到的包太小
            int iMoveCount=m_pRecvTail-m_pRecvHead;
            memmove(m_pRecvBuff,m_pRecvHead,iMoveCount);
            m_pRecvHead=m_pRecvBuff;
            m_pRecvTail=m_pRecvHead+iMoveCount;
            break;
        }
        //读包长度
        char *pTemp=m_pRecvHead;
        int offset=FuncTool::ReadShort(pTemp,header.m_iPackLen);
        if(iLen<header.m_iPackLen)
        {
            //收到的包太小
            int iMoveCount=m_pRecvTail-m_pRecvHead;
            memmove(m_pRecvBuff,m_pRecvHead,iMoveCount);
            m_pRecvHead=m_pRecvBuff;
            m_pRecvTail=m_pRecvHead+iMoveCount;
            break;
        }
        //读客户端下标
        pTemp+=offset;
        offset=FuncTool::ReadInt(pTemp,header.m_iClientIndex);

        //读取消息类型
        pTemp+=offset;
        offset=FuncTool::ReadShort(pTemp,header.m_iCmdId);

        //读取确认号
        int iMsgSeq;
        pTemp+=offset;
        offset=FuncTool::ReadInt(pTemp,iMsgSeq);
        sAckSeq.insert(iMsgSeq);
        //printf("recv one ack,seq is %d\n",iMsgSeq);

        //准备读下一个包
        m_pRecvHead+=header.m_iPackLen;
        iLen-=header.m_iPackLen;
    }
    //printf("recv server ack num is %d\n",(int)sAckSeq.size());
    //单个确认直接删除对应消息
    if(m_iConfirmLevel==PRODUCER_SINGLE_ACK)
    {
        list<CreatePublishMessage>::iterator itr=m_NeedAckMsgList.begin();
        while(itr!=m_NeedAckMsgList.end())
        {
            int iMsgSeq=itr->GetMsgSeq();
            if(sAckSeq.find(iMsgSeq)!=sAckSeq.end())
            {
                m_NeedAckMsgList.erase(itr++);
                //printf("erease one ack msg,imsg seq is %d\n",iMsgSeq);
            }
            else
            {
                itr++;
            }
        }
    }
    //批量确认则删除所有序号小于等于该序号的消息
    if(m_iConfirmLevel==PRODUCER_MUTIL_ACK)
    {
        unordered_set<int>::iterator itr1=sAckSeq.begin();
        while(itr1!=sAckSeq.end())
        {
            int iAckSeq=*itr1++;
            list<CreatePublishMessage>::iterator itr2=m_NeedAckMsgList.begin();
            while(itr2!=m_NeedAckMsgList.end())
            {
                if(itr2->GetMsgSeq()<=iAckSeq)
                {
                    m_NeedAckMsgList.erase(itr2++);
                }
                else
                {
                    itr2++;
                }
            }
        }
    }
    return SUCCESS;
}



int Producer::ReSendMsg()
{
    struct timeval tNow;
    gettimeofday(&tNow,NULL);
    //从待确认链表中找到对应消息进行移除
    //printf("begain resend msg ...\n");
    list<CreatePublishMessage>::iterator itr=m_NeedAckMsgList.begin();
    while(itr!=m_NeedAckMsgList.end())
    {
        CreatePublishMessage oMsg=*itr;
        //获取等待确认花费时间
        struct timeval tSaveTime=oMsg.GetSaveTime();
        int iWaitTime=((tNow.tv_sec-tSaveTime.tv_sec)*1000000+(tNow.tv_usec-tSaveTime.tv_usec))/1000;
        //3秒未确认则重发
        if(iWaitTime>=3*1000*1000)
        {
            //printf("resend one timeout msg ...\n");
            char pBuffer[MAX_CLINT_PACKAGE_LENGTH];
            int iLen=MAX_CLINT_PACKAGE_LENGTH;
            oMsg.GetMessagePack(pBuffer,&iLen);
            oMsg.SetSaveTime();
            m_NeedAckMsgList.push_back(oMsg);
            m_NeedAckMsgList.erase(itr++);
            //发送数据
            int ret=SendOnePack(pBuffer,iLen);
            if(ret!=Client::SUCCESS)
            {
                snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Server closed!");
                close(m_iSockfd);
                m_bBuild=false;
                return ERR_CLIENT_PUBLISH_MSG;
            }
        }
        else
        {
            break;
        }
    }
    //printf("resend msg end...\n");
    return SUCCESS;
}

int Consumer::RecvMessage(const string &istrQueueName,char *opBuffer,int *iopLen,unsigned char iConfirmLevel/*=CONSUMER_NO_ACK*/)
{
    if(m_bBuild==false)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection not build!");
        return ERR_CLIENT_BUILD_CONNECT;
    }
    CreateRecvMessage oMsg(istrQueueName,iConfirmLevel);
    //获取消息包
    char pMsgPack[MAX_CLINT_PACKAGE_LENGTH];
    int iPackLen=MAX_CLINT_PACKAGE_LENGTH;
    memset(pMsgPack,0,sizeof(pMsgPack));
    int ret=0;
    if((ret=oMsg.GetMessagePack(pMsgPack,&iPackLen))!=Message::SUCCESS)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Invaild parms!");
        return ERR_CLIENT_REV_MSG;
    }
    //发送数据
    if(SendOnePack(pMsgPack,iPackLen)!=Client::SUCCESS)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Server closed!");
        close(m_iSockfd);
        m_bBuild=false;
        return ERR_CLIENT_REV_MSG;
    }
    //接收回包
    unsigned short iReplyType=0;
    if(RecvOnePack(opBuffer,iopLen,iReplyType)!=Client::SUCCESS)
    {
        return ERR_CLIENT_REV_MSG;
    }
    //若是服务器回复消息，说明出错
    if(iReplyType==CMD_SERVER_REPLY_MESSAGE)
    {
        //解析回复包
        char *pTemp=opBuffer;
        bool bSucceed=true;
        int offset=FuncTool::ReadBool(pTemp,bSucceed);
        pTemp+=offset;
        offset=FuncTool::ReadBuf(pTemp,m_pErrMsg,sizeof(m_pErrMsg));
        return ERR_CLIENT_REV_MSG;
    }
    else
    {
        //读确认级别
        char *pTemp=opBuffer;
        int offset=FuncTool::ReadByte(pTemp,iConfirmLevel);
        pTemp+=offset;
        *iopLen=*iopLen-offset;
        //读取消息序号
        int iMsgSeq;
        offset=FuncTool::ReadInt(pTemp,iMsgSeq);
        pTemp+=offset;
        *iopLen=*iopLen-offset;
        memcpy(opBuffer,pTemp,*iopLen);
        opBuffer[*iopLen]='\0';
        
        //若是自动确认，则进行确认
        if(iConfirmLevel==CONSUMER_AUTO_ACK)
        {
            ClientAckMessage oMsg(iConfirmLevel,iMsgSeq);
            memset(pMsgPack,0,sizeof(pMsgPack));
            if((ret=oMsg.GetMessagePack(pMsgPack,&iPackLen))!=Message::SUCCESS)
            {
                snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Send ack failed!");
                return ERR_CLIENT_SEND_ACK;
            }
            //发送数据
            if(SendOnePack(pMsgPack,iPackLen)!=Client::SUCCESS)
            {
                snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Server closed!");
                close(m_iSockfd);
                m_bBuild=false;
                return ERR_CLIENT_REV_MSG;
            }
        }
        else if(iConfirmLevel==CONSUMER_SINGLE_ACK||iConfirmLevel==CONSUMER_MUTIL_ACK)
        {
            //手动确认则保存确认级别和确认号
            m_iConfirmLevel=iConfirmLevel;
            m_iAckSeq=iMsgSeq;
        }
        //若消息是之前已经收到的
        if(m_SeqSet.find(iMsgSeq)!=m_SeqSet.end())
        {
            snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Recved repeat message,message is %s!",opBuffer);
            return ERR_CLIENT_REV_MSG;
        }
        if(iConfirmLevel>0)
        {
            m_SeqSet.insert(iMsgSeq);
        }
        return SUCCESS;
    }
}

int Consumer::CreateSubscribe(const string &istrQueueName,unsigned char iConfirmLevel/*=0*/)
{
    if(m_bBuild==false)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection not build!");
        return ERR_CLIENT_BUILD_CONNECT;
    }
     CreateSubscribeMessage oMsg(istrQueueName,iConfirmLevel);
    //获取消息包
    char pMsgPack[MAX_CLINT_PACKAGE_LENGTH];
    int iPackLen=MAX_CLINT_PACKAGE_LENGTH;
    memset(pMsgPack,0,sizeof(pMsgPack));
    int ret=0;
    if((ret=oMsg.GetMessagePack(pMsgPack,&iPackLen))!=Message::SUCCESS)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Invaild parms!");
        return ERR_CLIENT_CREATE_SUBSCRIBE;
    }
    //发送数据
    SendOnePack(pMsgPack,iPackLen);

    //接收回复
    char pBuffer[MAX_SERVER_PACKAGE_LENGTH];
    int iLen=MAX_SERVER_PACKAGE_LENGTH;
    memset(pBuffer,0,sizeof(pBuffer));
    unsigned short iReplyType=0;
    if((ret=RecvOnePack(pBuffer,&iLen,iReplyType))!=Client::SUCCESS||iReplyType!=CMD_SERVER_REPLY_MESSAGE)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"reply error!");
        return ERR_CLIENT_CREATE_SUBSCRIBE;
    }
    //解析回复包
    char *pTemp=pBuffer;
    bool bSucceed=true;
    int offset=FuncTool::ReadBool(pTemp,bSucceed);
    if(!bSucceed)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"subsrcibe failed!");
        pTemp+=offset;
        offset=FuncTool::ReadBuf(pTemp,m_pErrMsg,sizeof(m_pErrMsg));
        return ERR_CLIENT_CREATE_SUBSCRIBE;
    }
    return SUCCESS;
}

int Consumer::ConsumeMessage(char *opBuffer,int *iopLen)
{
    if(m_bBuild==false)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection not build!");
        return ERR_CLIENT_BUILD_CONNECT;
    }
    //接收数据包
    unsigned short iReplyType=0;
    if(RecvOnePack(opBuffer,iopLen,iReplyType)!=Client::SUCCESS)
    {
        return ERR_CLIENT_REV_MSG;
    }
    //读确认级别
    char *pTemp=opBuffer;
    unsigned char confirmLevel;
    int offset=FuncTool::ReadByte(pTemp,confirmLevel);
    pTemp+=offset;
    *iopLen=*iopLen-offset;
    //读取消息序号
    int iMsgSeq;
    offset=FuncTool::ReadInt(pTemp,iMsgSeq);
    pTemp+=offset;
    *iopLen=*iopLen-offset;
    memcpy(opBuffer,pTemp,*iopLen);
    opBuffer[*iopLen]='\0';
    
    //若是自动确认，则进行确认
    if(confirmLevel==CONSUMER_AUTO_ACK&&iMsgSeq!=-1)
    {
        ClientAckMessage oMsg(confirmLevel,iMsgSeq);
        char pMsgPack[MAX_CLINT_PACKAGE_LENGTH];
        int iPackLen=MAX_CLINT_PACKAGE_LENGTH;
        memset(pMsgPack,0,sizeof(pMsgPack));
        int ret=0;
        if((ret=oMsg.GetMessagePack(pMsgPack,&iPackLen))!=Message::SUCCESS)
        {
            snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Send ack failed!");
            return ERR_CLIENT_SEND_ACK;
        }
        //printf("send client ack,msg seq is %d\n",iMsgSeq);
        //发送数据
        if(SendOnePack(pMsgPack,iPackLen)!=Client::SUCCESS)
        {
            snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Send ack failed!");
            return ERR_CLIENT_SEND_ACK;
        }
    }
    else if(confirmLevel==CONSUMER_SINGLE_ACK||confirmLevel==CONSUMER_MUTIL_ACK)
    {
        //手动确认则保存确认级别和确认号
        m_iConfirmLevel=confirmLevel;
        m_iAckSeq=iMsgSeq;
    }
    //若消息是之前已经收到的
    if(m_SeqSet.find(iMsgSeq)!=m_SeqSet.end())
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Recved repeat message,message is %s!",opBuffer);
        return ERR_CLIENT_REV_MSG;
    }
    if(confirmLevel>0)
    {
        m_SeqSet.insert(iMsgSeq);
    }
    return SUCCESS;
}

int Consumer::CancelSubscribe(const string &istrName)
{
    if(m_bBuild==false)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Connection not build!");
        return ERR_CLIENT_BUILD_CONNECT;
    }
    DeleteQueueMessage oMsg(istrName);
    //获取消息包
    char pMsgPack[MAX_CLINT_PACKAGE_LENGTH];
    int iPackLen=MAX_CLINT_PACKAGE_LENGTH;
    memset(pMsgPack,0,sizeof(pMsgPack));
    int ret=0;
    if((ret=oMsg.GetMessagePack(pMsgPack,&iPackLen))!=Message::SUCCESS)
    {
        snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Invaild parms!");
        return ERR_CLIENT_CREATE_SUBSCRIBE;
    }
    //发送数据
    SendOnePack(pMsgPack,iPackLen);

    //接收回复
    char pBuffer[MAX_SERVER_PACKAGE_LENGTH];
    int iLen=MAX_SERVER_PACKAGE_LENGTH;
    memset(pBuffer,0,sizeof(pBuffer));
    unsigned short iReplyType=0;
    if((ret=RecvOnePack(pBuffer,&iLen,iReplyType))!=Client::SUCCESS||iReplyType!=CMD_SERVER_REPLY_MESSAGE)
    {
        return ERR_CLIENT_CREATE_SUBSCRIBE;
    }
    //解析回复包
    char *pTemp=pBuffer;
    bool bSucceed=true;
    int offset=FuncTool::ReadBool(pTemp,bSucceed);
    if(!bSucceed)
    {
        pTemp+=offset;
        offset=FuncTool::ReadBuf(pTemp,m_pErrMsg,sizeof(m_pErrMsg));
        return ERR_CLIENT_CREATE_SUBSCRIBE;
    }
    return SUCCESS;
}


int Consumer::SendConsumerAck()
{
    if((m_iConfirmLevel==CONSUMER_SINGLE_ACK||m_iConfirmLevel==CONSUMER_MUTIL_ACK)&&m_iAckSeq!=-1)
    {
        ClientAckMessage oMsg(m_iConfirmLevel,m_iAckSeq);
        char pMsgPack[MAX_CLINT_PACKAGE_LENGTH];
        int iPackLen=MAX_CLINT_PACKAGE_LENGTH;
        memset(pMsgPack,0,sizeof(pMsgPack));
        int ret=0;
        if((ret=oMsg.GetMessagePack(pMsgPack,&iPackLen))!=Message::SUCCESS)
        {
            snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Send ack failed!");
            return ERR_CLIENT_SEND_ACK;
        }
        //printf("send client ack,msg seq is %d\n",iMsgSeq);
        //发送数据
        if(SendOnePack(pMsgPack,iPackLen)!=Client::SUCCESS)
        {
            snprintf(m_pErrMsg,sizeof(m_pErrMsg),"Send ack failed!");
            return ERR_CLIENT_SEND_ACK;
        }
    }
}