
/**
 * @Descripttion: 业务逻辑层头文件，接收接入层包，执行对应操作或者回包
 * @Author: readywang
 * @Date: 2020-07-21 11:26:48
 */
#ifndef INCLUDE_LOGIC_SERVER_H
#define INCLUDE_LOGIC_SERVER_H

#include<string>
#include<unordered_map>
#include<unordered_set>
#include<list>
#include<vector>
#include<queue>
#include<stdio.h>
#include"../shm_queue/shm_queue.h"
#include"../message/message.h"
using std::string;
using std::unordered_map;
using std::list; 
using std::vector;
using std::priority_queue;
using std::unordered_set;

//服务器回复内容
#define RE_CREATE_EXCHANGE_SUCCESS "Create exchange success!"
#define RE_CREATE_EXCHANGE_FAILED "Create exchange failed!"
#define RE_EXCHANGE_EXIST "Exchange already exist! name is "
#define RE_CREATE_QUEUE_SUCCESS "Create queue success!"
#define RE_CREATE_QUEUE_FAILED "Create queue failed!"
#define RE_QUEUE_EXIST "Queue already exist! name is "
#define RE_EXCHANGE_NOT_EXIST "Exchange not exist! name is "
#define RE_QUEUE_NOT_EXIST "Queue not exist! name is "
#define RE_QUEUE_NOT_SUBSCRIBE "Queue isn't subscribed! name is "
#define RE_QUEUE_IS_EMPTY "Queue is empty! name is "
#define RE_FIT_QUEUE_NOT_EXIST "Fit queue not exist! routing key is "
#define RE_BINDING_SUCCEED "Binding succeed! binding key is "
#define RE_BINDING_FAILED "Binding failed! binding key is "
#define RE_SUBSCRIBE_SUCCEED "Subscribe succeed! queue name is "
#define RE_SUBSCRIBE_FAILED "Subscribe failed! queue name is "
#define RE_DELETE_EXCHANGE_SUCCEED "Delete exchange succeed! exchange name is "
#define RE_DELETE_EXCHANGE_FAILED "Delete exchange failed! exchange name is "
#define RE_DELETE_QUEUE_SUCCEED "Delete queue succeed! queue name is "
#define RE_DELETE_QUEUE_FAILED "Delete queue failed! queue name is "
#define RE_CANCEL_SUBSCRIBE_SUCCEED "Cancel subscribe succeed! queue name is "
#define RE_CANCEL_SUBSCRIBE_FAILED "Cancel subscribe failed! queue name is "
#define RE_DURABLE_FAILED "Durable failed!"


namespace WSMQ
{
    //定义比较对象
    struct cmp
    {
        bool operator()(SeverStoreMessage* ipMsg1,SeverStoreMessage* ipMsg2)
        {
            return ipMsg1->GetPriority()<ipMsg2->GetPriority();
        }
    };
    //正常退出时的保存文件夹及路径
    const string PATH_STOP_RECORD="./stop_record/";
    const string FILE_STOP_SAVE_EXCHANGE="exchange";
    const string FILE_STOP_SAVE_QUEUE="queue";
    const string FILE_STOP_SAVE_MESSAGE="message";
    class MessageQueue
    {
    public:
        const static int SUCCESS=0;
        const static int ERROR=-1;
        
    public:
        MessageQueue(const string &istrName,short iPriority=-1,bool ibDurable=false,bool ibAutoDel=true);
        string GetQueueName(){return m_strQueueName;}
        void SetQueueName(const string &istrName){m_strQueueName=istrName;}
        short GetPriority(){return m_iPriority;}
        void SetPriority(short iPriority){m_iPriority=iPriority;}
        bool IsDurable(){return m_bDurable==true;}
        bool IsAutoDel(){return m_bAutoDel==true;}
        void SetConfirmLevel(unsigned char iConfirmLevel){m_iConfirmLevel=iConfirmLevel;}
        int GetConfirmLevel(){return m_iConfirmLevel;}
        bool IsEmpty(){return m_msgPriQueue.empty()&&m_messagesList.empty();}
        bool IsSubscribe(){return !m_vSubscribeConsumers.empty();}
        SeverStoreMessage * PopFrontMsg(bool ibPush=false);
        int AddMessage(SeverStoreMessage * ipMsg);
        int AddMessageFront(SeverStoreMessage * ipMsg);
        int ClearMessage();
        int AddSubscribe(int iCliIndex,unsigned char iConfirmLevel);
        int CancelSubscribe(int iCliIndex);
        int GetSubscribeSize(){return m_vSubscribeConsumers.size();}
        int SerializeSubcribeToString(char *ipBuffer,int &iBuffLen,bool ibIsCreate);
        int SetPushIndex(int iIndex){m_iPushIndex=iIndex;}
        int SetNextDaurableIndex(int iIndex){m_iDurableIndex=iIndex;}
        int GetNextDaurableIndex(){return m_iDurableIndex++;}
    protected:
        //队列名称
        string m_strQueueName;
        //队列优先级
        short m_iPriority;
        //队列是否需要持久化
        bool m_bDurable;
        //队列是否是自动删除的
        bool m_bAutoDel;
        //队列中的消息的确认级别（只针对push模式有效）
        unsigned char m_iConfirmLevel;
        //存放的普通消息
        list<SeverStoreMessage *>m_messagesList;
        //存放的优先消息
        priority_queue<SeverStoreMessage *,vector<SeverStoreMessage *>,cmp>m_msgPriQueue;
        //队列的订阅者
        vector<int>m_vSubscribeConsumers;
        //上次推送给了那一个订阅者，每次都从上次分配的下一个开始
        int m_iPushIndex;
        //持久化消息序号
        int m_iDurableIndex;
    };
    class Exchange
    {
    public:
        const static int SUCCESS=0;
        const static int ERROR=-1;
        const static int ERR_BINDING_NOT_EXIST=-800;
    public:
        Exchange(const string &istrName,int iExchangeType,bool ibDurable=false,bool ibAutoDel=true);
        string GetExchangeName(){return m_strExchangeName;}
        void SetExchangeName(const string& istrName){m_strExchangeName=istrName;}
        unsigned short GetExchangetype(){return m_iExchangeType;}
        void SetExchangetype(int iType){m_iExchangeType=iType;}
        bool IsDurable(){return m_bDurable==true;}
        bool IsAutoDel(){return m_bAutoDel==true;}
        int CreateBinding(const string &istrBindingKey,MessageQueue *ipMsgQueue);
        int FindBindingQueue(const string& istrRoutingKey,list<MessageQueue *>&oMsgQueueList);
        int DeleteBindingQueue(MessageQueue *ipMsgQueue);
        int GetBindingSize(){return m_mBindingQueues.size();}
        int SerializeBindingToString(char *ipBuffer,int &iBuffLen);
    protected:
        bool IsFit(const string& istrRoutingKey,const string& iStrBindingKey);
        //exchange 名称
        string m_strExchangeName;
        //exchange type
        unsigned short m_iExchangeType;
        //是否持久化
        bool m_bDurable;
        //是否自动删除
        bool m_bAutoDel;
        //根据binding key 绑定的queue
        unordered_map<string,list<MessageQueue*>>m_mBindingQueues;
    };

    class LogicServer
    {
    public:
        const static int SUCCESS=0;
        const static int ERROR=-1;
        const static int ERR_LOGIC_SERVER_RUN=-700;
        const static int ERR_LOGIC_SERVER_PROCESS=-701;
        const static int ERR_LOGIC_SERVER_CREATE_EXCHANGE=-703;
        const static int ERR_LOGIC_SERVER_CREATE_QUEUE=-704;
        const static int ERR_LOGIC_SERVER_CREATE_BINDING=-705;
        const static int ERR_LOGIC_SERVER_PUBLISH_MESSAGE=-706;
        const static int ERR_LOGIC_SERVER_RECV_MESSAGE=-707;
        const static int ERR_LOGIC_SERVER_CREATE_SUBSCRIBE=-708;
        const static int ERR_LOGIC_SERVER_DELETE_EXCHANGE=-709;
        const static int ERR_LOGIC_SERVER_DELETE_QUEUE=-710;
        const static int ERR_LOGIC_SERVER_CANCEL_SUBSCRIBE=-711;
        const static int ERR_LOGIC_SERVER_QUEUE_NOT_EXIST=-712;

    public:
        static LogicServer*GetInstance();
        static void Destroy();
        int Init();
        int Run();
    protected:
        int InitConf(const char *ipPath);
        LogicServer();
        ~LogicServer();
        int InitSigHandler();
        static void SigTermHandler(int iSig){ m_bStop = true; }
        int OnCreateQueue(char *ipBuffer,int iLen,string &ostrMsgBody);
        int OnCreateExchange(char *ipBuffer,int iLen,string &ostrMsgBody);
        int OnCreateBinding(char *ipBuffer,int iLen,string &ostrMsgBody);
        int OnPublishMessage(char *ipBuffer,int iLen,int iCliIndex,string &ostrMsgBody);
        int SendMultiAck();
        int OnRecvMessage(char *ipBuffer,int iLen,int iCliIndex,string &ostrMsgBody);
        int OnCreateSubscribe(char *ipBuffer,int iLen,int iCliIndex,string &ostrMsgBody);
        int OnDeleteExchange(char *ipBuffer,int iLen,string &ostrMsgBody);
        int OnDeleteQueue(char *ipBuffer,int iLen,string &ostrMsgBody);
        int OnCancelSubscribe(char *ipBuffer,int iLen,int iCliIndex,string &ostrMsgBody);
        int OnClientAck(char *ipBuffer,int iLen,int iCliIndex);
        int OnClientExit(int iClientIndex);
        //将消息队列数据推送到接入层
        int PushMessage();
        //处理从接入层接收到的数据
        int ProcessRecvData(char *ipBuffer,int iLen,int &opClientIndex,string &ostrMsgBody);
        //重发未确认消息
        int ReSendMsg();
        //停止时保存数据
        int OnStop();
        //重启时尝试读文件回复数据
        int OnInit();
        //初始化队列
        int OnInitQueue();
        //初始化exchange
        int OnInitExchange();
        //初始化msg
        int OnInitMsg();
        //根据消息下标在文件夹中找消息所在文件组名称
        string FindMessageFileName(int iMsgIndex,const char *ipPath);
        string ConvertIndexToString(int iIndex);
        //根据消息下标找到索引文件
        int FindPosInIndexFile(int iMsgIndex,const char *ipFile);
        //处理每一个数据文件
        int ProcessDataFile(const char *ipFile,int &oLastIndex,int iStartPos=0);
        //找到文件夹中所有的index文件名称
        int FindIndexFiles(const char *ipDir,vector<string>&ovIndexFiles);
        //处理持久化消息，进行恢复
        int ProcessDurableMsgFile(const char *ipFile);
    protected:
        //停止标志
        static bool m_bStop;
        //和接入层通信的消息队列
        ShmQueue *m_pQueueToConnect;
        ShmQueue *m_pQueueFromConnect;
        //和持久化层通信的消息队列
        ShmQueue *m_pQueueToPersis;
        ShmQueue *m_pQueueFromPersis;
        //根据名称保存的未订阅队列对象
        unordered_map<string,MessageQueue*>m_mUnSubcribeQueues;
        //根据名称保存的订阅队列对象
        unordered_map<string,MessageQueue*>m_mSubcribeQueues;
        //根据名称保存的exchange对象
        unordered_map<string,Exchange*>m_mExchanges;
        //保存的等待消费者确认的消息链表
        list<SeverStoreMessage *>m_NeedAckMessageList;
        //待消费者确认的消息编号
        unsigned int m_iAckSeq;
        //生产者发来的消息集合，格式为"生产者下标#消息编号"用于去重
        unordered_set<string>m_producerMsgSeqs;
        //批量确认时保存每一个客户发来的最新序号
        unordered_map<int,int>m_producerLastMsgSeq;
        static LogicServer *m_pLogicServer;
        int m_iRecvCount;
        int m_iSendCount;
    };
    LogicServer *LogicServer::m_pLogicServer=NULL;
    bool LogicServer::m_bStop=true;
}
#endif //INCLUDE_LOGIC_SERVER_H