
/**
 * @Descripttion: 客户端代码，包括生产和消费
 * @Author: readywang
 * @Date: 2020-07-20 15:57:41
 */
#ifndef INCLUDE_CLIENT_H
#define INCLUDE_CLIENT_H

#include"../common_def/comdef.h"
#include"../message/message.h"
#include"../ini_file/ini_file.h"
#include"../mq_util/mq_util.h"
#include<string>
#include<list>
#include<unordered_set>
#include<stdint.h>
using std::string;
using std::unordered_set;
using std::list;

namespace WSMQ
{
    class Client
    {
    public:
        const static int SUCCESS=0;
        const static int ERROR=-1;
        const static int ERR_CLIENT_BUILD_CONNECT=-600;
        const static int ERR_CLIENT_SEND_PACK=-601;
        const static int ERR_CLIENT_CREATE_EXCHANGE=-602;
        const static int ERR_CLIENT_CREATE_QUEUE=-603;
        const static int ERR_CLIENT_CREATE_BINDING=-604;
        const static int ERR_CLIENT_PUBLISH_MSG=-605;
        const static int ERR_CLIENT_REV_MSG=-606;
        const static int ERR_CLIENT_CONNECT_CLOSED=-607;
        const static int ERR_CLIENT_CREATE_SUBSCRIBE=-608;
        const static int ERR_CLIENT_REV_PACK=-609;
        const static int ERR_CLIENT_SEND_ACK=-610;
        const static int ERR_CLIENT_DELETE_EXCHANGE=-611;
        const static int ERR_CLIENT_DELETE_QUEUE=-612;
        const static int ERR_SEVER_CLOSED=-613;
        const static int ERR_SOCKET_SEND_BUFF_FULL=-614;
    public:
        Client();
        ~Client();
        int BuildConnection(const char *ipSeverIp=SERVER_DEFAULT_IP_ADDR,unsigned short iServerPort=SERVER_DEFAULT_PORT);
        int CreateExchange(const string &istrName,unsigned short iExchangeType,bool ibDurable=false,bool ibAutoDel=true);
        int CreateQueue(const string &istrName,short iPriority=-1,bool ibDurable=false,bool ibAutoDel=true);
        int CreateBinding(const string &istrExName, const string &istrQueueName, const string &istrKey);
        int DeleteExchange(const string &istrName);
        int DeleteQueue(const string &istrName);
        int GetSockfd(){return m_iSockfd;}
        void SetConfirmLevel(unsigned char iConfirmLevel){m_iConfirmLevel=iConfirmLevel;}
        unsigned char GetConfirmLevel(){return m_iConfirmLevel;}
        const char* GetErrMsg(){return m_pErrMsg;}
    protected:
        int WaitServerReply();
        int SendOnePack(const char *ipBuffer,int iLen);//发送数据包
        int SendPack();
        int RecvOnePack(char *opBuffer,int *iopLen,unsigned short &oRelyType);//接收一个回复包
        int RecvPack();
        int m_iSockfd;
        int m_bBuild;
        unsigned char m_iConfirmLevel;
        char m_pErrMsg[256];
        char *m_pRecvBuff;
        char *m_pRecvHead;
        char *m_pRecvTail;
        char *m_pRecvEnd;
        char *m_pSendBuff;
        char *m_pSendHead;
        char *m_pSendTail;
        char *m_pSendEnd;
    };
    class Producer:public Client
    {
    public:
        Producer();
        ~Producer();
        
        int PuslishMessage(const string &istrExName,const string &istrKey,const string &istrMsgBody, short iPriority=-1,bool ibDurable=false);
        int RecvServerAck();
        int ReSendMsg();
    protected:
        int m_iMsgSeq;
        list<CreatePublishMessage >m_NeedAckMsgList;
    };

    class Consumer:public Client
    {
    public:
        Consumer(){m_iAckSeq=-1;};
        ~Consumer(){};
        int RecvMessage(const string &istrQueueName,char *opBuffer,int *iopLen,unsigned char iConfirmLevel=CONSUMER_NO_ACK);
        int CreateSubscribe(const string &istrQueueName,unsigned char iConfirmLevel=0);
        int ConsumeMessage(char *opBuffer,int *iopLen);
        int CancelSubscribe(const string &istrName);
        int SendConsumerAck();
    protected:
        unordered_set<int>m_SeqSet;
        int m_iAckSeq;
    };

    static int InitConf(const char *ipPath)
    {
        // if(!FuncTool::IsFileExist(ipPath))
        // {
        //     return -1;
        // }
        // CIniFile objIniFile(ipPath);
        // objIniFile.GetInt("MQ_CONF", "MaxCliPackSize", 0, &MAX_CLINT_PACKAGE_LENGTH);
        // objIniFile.GetInt("MQ_CONF", "MaxSrvPackSize", 0, &MAX_SERVER_PACKAGE_LENGTH);
        // objIniFile.GetInt("MQ_CONF", "MaxNameLength", 0, &MAX_NAME_LENGTH);
        // objIniFile.GetString("MQ_CONF", "ServerIP", "",SERVER_DEFAULT_IP_ADDR , sizeof(SERVER_DEFAULT_IP_ADDR));
        // objIniFile.GetInt("MQ_CONF", "ServerPort", 0, (int *)&SERVER_DEFAULT_PORT);
        // objIniFile.GetInt("MQ_CONF", "EpollCount", 0, &CLIENT_EPOLL_COUNT);
        // objIniFile.GetInt("MQ_CONF", "SockSendBufSize", 32, &SOCK_SEND_BUFF_SIZE);
        // objIniFile.GetInt("MQ_CONF", "SockRecvBufSize", 0, &SOCK_RECV_BUFF_SIZE);
        // objIniFile.GetInt("MQ_CONF", "ShmConLogicKey", 0, &CONNECT_TO_LOGIC_KEY);
        // objIniFile.GetInt("MQ_CONF", "ShmLogicConKey", 0, &LOGIC_TO_CONNECT_KEY);
        // objIniFile.GetInt("MQ_CONF", "ShmPersisLogicKey", 0, &PERSIS_TO_LOGIC_KEY);
        // objIniFile.GetInt("MQ_CONF", "ShmLogicPersisKey", 0, &LOGIC_TO_PERSIS_KEY);
        // objIniFile.GetInt("MQ_CONF", "ShmQueueSize", 0, &SHM_QUEUE_SIZE);
        // objIniFile.GetString("MQ_CONF", "PersisExchangePath", "",DEFAULT_EXCHANGE_PATH , sizeof(DEFAULT_EXCHANGE_PATH));
        // objIniFile.GetString("MQ_CONF", "PersisQueuePath", "",DEFAULT_QUEUE_PATH , sizeof(DEFAULT_QUEUE_PATH));
        // objIniFile.GetInt("MQ_CONF", "PersisMsgFileSize", 0, &DURABLE_MESSAGE_FILE_SIZE);
         return 0;
    }
}

#endif //INCLUDE_CLIENT_H