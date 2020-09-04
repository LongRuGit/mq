//c/c++
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fstream>
#include<algorithm>
//linux
#include<signal.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<dirent.h>
#include<fcntl.h>
#include<sys/mman.h>
//user define
#include"../mq_util/mq_util.h"
#include"../common_def/comdef.h"
#include "logicServer.h"
#include "../timer/timer.h"
#include"../message/message.h"
#include"../logger/logger.h"
#include"../ini_file/ini_file.h"
using namespace WSMQ;
using std::to_string;
using std::fstream;
//全局对象
Logger LogicServerLogger;



MessageQueue::MessageQueue(const string &istrName,short iPriority/*=-1*/,bool ibDurable/*=false*/,bool ibAutoDel/*=true*/)
:m_strQueueName(istrName),m_iPriority(iPriority),m_bDurable(ibDurable),m_bAutoDel(ibAutoDel)
{
    m_messagesList.clear();
    m_vSubscribeConsumers.clear();
    m_iPushIndex=-1;
    m_iConfirmLevel=0;
    m_iDurableIndex=0;
}

SeverStoreMessage *MessageQueue::PopFrontMsg(bool ibPush/*=false*/)
{
    if(ibPush&&m_vSubscribeConsumers.empty())
    {
        return NULL;
    }
    if(m_msgPriQueue.empty()&&m_messagesList.empty())
    {
        return NULL;
    }
    //若优先消息不为空，则推送优先消息
    SeverStoreMessage *pFrontMsg=NULL;
    if(!m_msgPriQueue.empty())
    {
        pFrontMsg=m_msgPriQueue.top();
        m_msgPriQueue.pop();
    }
    else 
    {
        pFrontMsg=m_messagesList.front();
        m_messagesList.pop_front();
    }

    //若消息是用来推送，从订阅者中选择一个接收
    if(ibPush)
    {
        m_iPushIndex++;
        if(m_iPushIndex>=(int)m_vSubscribeConsumers.size())
        {
            m_iPushIndex=0;
        }
        pFrontMsg->SetClientIndex(m_vSubscribeConsumers[m_iPushIndex]);
    }    
    return pFrontMsg;
}

int MessageQueue::AddMessage(SeverStoreMessage * ipMsg)
{
    //若队列和消息都支持优先级，则作为优先级消息存放，否则作为普通消息
    if(m_iPriority>0&&ipMsg->GetPriority()>0)
    {
        LogicServerLogger.WriteLog(mq_log_info,"put msg into priority queue! m_iPriority is %d is %d",m_iPriority,ipMsg->GetPriority());
        LogicServerLogger.Print(mq_log_info,"put msg into priority queue! m_iPriority is %d is %d",m_iPriority,ipMsg->GetPriority());
        if(ipMsg->GetPriority()>m_iPriority)
        {
            ipMsg->SetPriority(m_iPriority);
        }
        m_msgPriQueue.push(ipMsg);
    }
    else
    {
        m_messagesList.push_back(ipMsg);
    }
    return SUCCESS;
}

int MessageQueue::AddMessageFront(SeverStoreMessage * ipMsg)
{
    //若队列和消息都支持优先级，则作为优先级消息存放，否则作为普通消息
    if(m_iPriority>0&&ipMsg->GetPriority()>0)
    {
        LogicServerLogger.WriteLog(mq_log_info,"put msg into priority queue! m_iPriority is %d is %d",m_iPriority,ipMsg->GetPriority());
        LogicServerLogger.Print(mq_log_info,"put msg into priority queue! m_iPriority is %d is %d",m_iPriority,ipMsg->GetPriority());
        if(ipMsg->GetPriority()>m_iPriority)
        {
            ipMsg->SetPriority(m_iPriority);
        }
        m_msgPriQueue.push(ipMsg);
    }
    else
    {
        m_messagesList.push_front(ipMsg);
    }
    return SUCCESS;
}

int MessageQueue::ClearMessage()
{
    SeverStoreMessage *pFrontMsg=NULL;
    while(!m_msgPriQueue.empty())
    {
        pFrontMsg=m_msgPriQueue.top();
        m_msgPriQueue.pop();
        delete pFrontMsg;
    }
    while(!m_messagesList.empty()) 
    {
        pFrontMsg=m_messagesList.front();
        m_messagesList.pop_front();
        delete pFrontMsg;
    }
    return SUCCESS;
}

int MessageQueue::AddSubscribe(int iCliIndex,unsigned char iConfirmLevel)
{
    bool bExist=false;
    for(int i=0;i<(int)m_vSubscribeConsumers.size();++i)
    {
        if(m_vSubscribeConsumers[i]==iCliIndex)
        {
            bExist=true;
        }
    }
    if(!bExist)
    {
        m_vSubscribeConsumers.push_back(iCliIndex);
    }
    m_iConfirmLevel=iConfirmLevel;
    return SUCCESS;
}

int MessageQueue::CancelSubscribe(int iCliIndex)
{
    for(int i=0;i<(int)m_vSubscribeConsumers.size();++i)
    {
        if(m_vSubscribeConsumers[i]==iCliIndex)
        {
            m_vSubscribeConsumers.erase(m_vSubscribeConsumers.begin()+i);
        }
    }
    if(m_vSubscribeConsumers.empty())
    {
        m_iConfirmLevel=0;
    }
    return SUCCESS;
}

int MessageQueue::SerializeSubcribeToString(char *ipBuffer,int &iBuffLen,bool ibIsCreate)
{
    unsigned short iLen=sizeof(unsigned short)+sizeof(unsigned short)+MAX_NAME_LENGTH+sizeof(m_iConfirmLevel)+sizeof(unsigned short);
    if(!m_vSubscribeConsumers.empty())
    {
        iLen+=m_vSubscribeConsumers.size()*sizeof(int);
    }
    if(iLen>iBuffLen)
    {
        return ERROR;
    }
    char *pBuff=ipBuffer;
    int offset=FuncTool::WriteShort(pBuff,iLen);
    pBuff+=offset;
    if(ibIsCreate)
    {
        offset=FuncTool::WriteShort(pBuff,CMD_CREATE_SUBCRIBE);
    }
    else
    {
        offset=FuncTool::WriteShort(pBuff,CMD_CANCEL_SUBCRIBE);
    }
    pBuff+=offset;
    offset=FuncTool::WriteBuf(pBuff,m_strQueueName.c_str(),sizeof(m_strQueueName));
    pBuff+=MAX_NAME_LENGTH;
    offset=FuncTool::WriteByte(pBuff,m_iConfirmLevel);
    pBuff+=offset;
    offset=FuncTool::WriteShort(pBuff,(unsigned short)m_vSubscribeConsumers.size());
    for(int i=0;i<m_vSubscribeConsumers.size();++i)
    {
        pBuff+=offset;
        offset=FuncTool::WriteInt(pBuff,m_vSubscribeConsumers[i]);
    }
    iBuffLen=iLen;
    return SUCCESS;
}

Exchange::Exchange(const string &istrName,int iExchangeType,bool ibDurable/*=false*/,bool ibAutoDel/*=true*/)
:m_strExchangeName(istrName),m_iExchangeType(iExchangeType),m_bDurable(ibDurable),m_bAutoDel(ibAutoDel)
{
}

int Exchange:: CreateBinding(const string &istrBindingKey,MessageQueue *ipMsgQueue)
{
    //查找是否存在现有绑定
    if(m_mBindingQueues.find(istrBindingKey)!=m_mBindingQueues.end())
    {
        list<MessageQueue *>queueList=m_mBindingQueues[istrBindingKey];
        list<MessageQueue *>::iterator itr=queueList.begin();
        while(itr!=queueList.end()&&*itr!=ipMsgQueue)
        {
            ++itr;
        }
        if(itr==queueList.end())
        {
            queueList.push_back(ipMsgQueue);
        }
    }
    else
    {
        m_mBindingQueues[istrBindingKey].push_back(ipMsgQueue);
    }
    return SUCCESS;
}

bool Exchange::IsFit(const string& istrRoutingKey,const string& iStrBindingKey)
{
    //将字符串用.进行分割
    vector<string>vStrRoutingKey;
    vector<string>vStrBindingKey;
    int lo=0;
    int hi=0;
    while(hi<=(int)istrRoutingKey.size())
    {
        if(istrRoutingKey[hi]!='.'&&istrRoutingKey[hi]!='\0')
        {
            ++hi;
        }
        else
        {
            string strSub=istrRoutingKey.substr(lo,hi-lo);
            if(!strSub.empty())
            {
                vStrRoutingKey.push_back(strSub);
            }
            lo=++hi;
        }
    }
    lo=0;
    hi=0;
    while(hi<=(int)iStrBindingKey.size())
    {
        if(iStrBindingKey[hi]!='.'&&iStrBindingKey[hi]!='\0')
        {
            ++hi;
        }
        else
        {
            string strSub=iStrBindingKey.substr(lo,hi-lo);
            if(!strSub.empty())
            {
                vStrBindingKey.push_back(strSub);
            }
            lo=++hi;
        }
    }
    //dp进行匹配判断
    int iLen1=vStrBindingKey.size();
    int iLen2=vStrRoutingKey.size();
    vector< vector<bool> >dp(iLen1+1,vector<bool>(iLen2+1,false));
    dp[0][0]=true;
    for(int i=1;i<=iLen1;++i)
    {
        for(int j=1;j<=iLen2;++j)
        {
            if(vStrRoutingKey[j-1]=="*"||vStrBindingKey[i-1]==vStrRoutingKey[j-1])
            {
                dp[i][j]=dp[i-1][j-1];
            }
            if(vStrRoutingKey[j-1]=="#")
            {
                dp[i][j]=dp[i-1][j]||dp[i][j-1];
            }
        }
    }
    return dp[iLen1][iLen2];
}

int Exchange::FindBindingQueue(const string& istrRoutingKey,list<MessageQueue *>&oMsgQueueList)
{
     //根据exchange 类型进行处理
    if(m_iExchangeType==EXCHANGE_TYPE_DIRECT)
    {
        //直接转发给key匹配队列
        if(m_mBindingQueues.find(istrRoutingKey)==m_mBindingQueues.end())
        {
            return ERR_BINDING_NOT_EXIST;
        }
        oMsgQueueList=m_mBindingQueues[istrRoutingKey];
    }
    else if(m_iExchangeType==EXCHANGE_TYPE_FANOUT)
    {
        //转发给所有绑定队列
        if(m_mBindingQueues.empty())
        {
            return ERR_BINDING_NOT_EXIST;
        }
        unordered_map<string,list<MessageQueue *>>::iterator itr1=m_mBindingQueues.begin();
        while(itr1!=m_mBindingQueues.end())
        {
            list<MessageQueue *>queueList=itr1->second;
            list<MessageQueue *>::iterator itr2=queueList.begin();
            while(itr2!=queueList.end())
            {
                oMsgQueueList.push_back(*itr2++);
            }
            ++itr1;
        }
    }
    else if(m_iExchangeType==EXCHANGE_TYPE_TOPIC)
    {
        //匹配查询
        if(m_mBindingQueues.empty())
        {
            return ERR_BINDING_NOT_EXIST;
        }
        unordered_map<string,list<MessageQueue *>>::iterator itr1=m_mBindingQueues.begin();
        while(itr1!=m_mBindingQueues.end())
        {
            string strBindingKey=itr1->first;
            if(IsFit(istrRoutingKey,strBindingKey))
            {
                list<MessageQueue *>queueList=itr1->second;
                list<MessageQueue *>::iterator itr2=queueList.begin();
                while(itr2!=queueList.end())
                {
                    oMsgQueueList.push_back(*itr2++);
                }
            }
            ++itr1;
        }
    }
    return SUCCESS;
}

int Exchange::DeleteBindingQueue(MessageQueue *ipMsgQueue)
{
    unordered_map<string,list<MessageQueue *>>::iterator itr1=m_mBindingQueues.begin();
    while(itr1!=m_mBindingQueues.end())
    {
        list<MessageQueue *>queueList=itr1->second;
        list<MessageQueue *>::iterator itr2=queueList.begin();
        while(itr2!=queueList.end())
        {
            if(*itr2==ipMsgQueue)
            {
                queueList.erase(itr2++);
            }
            else
            {
                itr2++;
            }
        }
        if(queueList.empty())
        {
            m_mBindingQueues.erase(itr1++);
        }
        else
        {
            ++itr1;
        }
    }
    return SUCCESS;
}

int Exchange::SerializeBindingToString(char *ipBuffer,int &iBuffLen)
{
    unsigned short iLen=sizeof(unsigned short)+sizeof(unsigned short)+MAX_NAME_LENGTH+sizeof(unsigned short);
    unordered_map<string,list<MessageQueue *>>::iterator itr1=m_mBindingQueues.begin();
    while(itr1!=m_mBindingQueues.end())
    {
        list<MessageQueue *>queueList=itr1->second;
        list<MessageQueue *>::iterator itr2=queueList.begin();
        iLen+=MAX_NAME_LENGTH;
        iLen+=sizeof(unsigned short);
        while(itr2!=queueList.end())
        {
            iLen+=MAX_NAME_LENGTH;
            ++itr2;
        }
        ++itr1;
    }
    if(iLen>iBuffLen)
    {
        return ERROR;
    }
    char *pBuff=ipBuffer;
    int offset=FuncTool::WriteShort(pBuff,iLen);
    pBuff+=offset;
    offset=FuncTool::WriteShort(pBuff,CMD_CREATE_BINDING);
    
    pBuff+=offset;
    offset=FuncTool::WriteBuf(pBuff,m_strExchangeName.c_str(),m_strExchangeName.size());
    pBuff+=MAX_NAME_LENGTH;
    
    offset=FuncTool::WriteShort(pBuff,(unsigned short)(m_mBindingQueues.size()));
    pBuff+=offset;
    itr1=m_mBindingQueues.begin();
    while(itr1!=m_mBindingQueues.end())
    {
        offset=FuncTool::WriteBuf(pBuff,itr1->first.c_str(),sizeof(itr1->first));
        pBuff+=MAX_NAME_LENGTH;
        list<MessageQueue *>queueList=itr1->second;
        offset=FuncTool::WriteShort(pBuff,(unsigned short)queueList.size());
        pBuff+=offset;
        list<MessageQueue *>::iterator itr2=queueList.begin();
        while(itr2!=queueList.end())
        {
            string strQueueName=(*itr2)->GetQueueName();
            offset=FuncTool::WriteBuf(pBuff,strQueueName.c_str(),sizeof(strQueueName));
            pBuff+=MAX_NAME_LENGTH;
            ++itr2;
        }
        ++itr1;
    }
    iBuffLen=iLen;
    return SUCCESS;
}

LogicServer::LogicServer()
{
    m_bStop=false;
    m_pQueueToConnect=NULL;
    m_pQueueFromConnect=NULL;
    m_pQueueToPersis=NULL;
    m_pQueueFromPersis=NULL;
    m_iAckSeq=0;
    //m_mUnSubcribeQueues.clear();
    //m_mSubcribeQueues.clear();
    //m_mExchanges.clear();
    m_iSendCount=0;
    m_iRecvCount=0;
}

LogicServer::~LogicServer()
{
    if(m_pQueueFromConnect)
    {
        delete m_pQueueFromConnect;
        m_pQueueFromConnect=NULL;
    }
    if(m_pQueueToConnect)
    {
        delete m_pQueueToConnect;
        m_pQueueToConnect=NULL;
    }
    if(m_pQueueToPersis)
    {
        delete m_pQueueToPersis;
        m_pQueueToPersis=NULL;
    }
    if(m_pQueueFromPersis)
    {
        m_pQueueFromPersis=NULL;
    }
}

LogicServer * LogicServer::GetInstance()
{
    if(m_pLogicServer==NULL)
    {
        m_pLogicServer=new LogicServer();
    }
    return m_pLogicServer;
}

void LogicServer::Destroy()
{
    if(m_pLogicServer)
    {
        delete m_pLogicServer;
        m_pLogicServer=NULL;
    }
}

int LogicServer::InitSigHandler()
{
    struct sigaction act;
    memset(&act,0,sizeof(act));
    act.sa_handler=SigTermHandler;
    sigaction(SIGINT,&act,NULL);
    sigaction(SIGTERM,&act,NULL);
    sigaction(SIGQUIT,&act,NULL);

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGSEGV);
    sigaddset(&set, SIGBUS);
    sigaddset(&set, SIGABRT);
    sigaddset(&set, SIGILL);
    sigaddset(&set, SIGFPE);
    sigprocmask(SIG_UNBLOCK,&set,NULL);
    return SUCCESS;
}

int LogicServer::InitConf(const char *ipPath)
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

int LogicServer::Init()
{
    if(InitConf(CONF_FILE_PATH)!=SUCCESS)
    {
        return ERROR;
    }
    InitSigHandler();
    //初始化和接入层的通信通道
    m_pQueueFromConnect=new ShmQueue();
    m_pQueueToConnect=new ShmQueue();
    m_pQueueFromConnect->Init(CONNECT_TO_LOGIC_KEY,SHM_QUEUE_SIZE);
    m_pQueueToConnect->Init(LOGIC_TO_CONNECT_KEY,SHM_QUEUE_SIZE);
    //初始化和持久化层的通信通道
    m_pQueueFromPersis=new ShmQueue();
    m_pQueueToPersis=new ShmQueue();
    m_pQueueFromPersis->Init(PERSIS_TO_LOGIC_KEY,SHM_QUEUE_SIZE);
    m_pQueueToPersis->Init(LOGIC_TO_PERSIS_KEY,SHM_QUEUE_SIZE);

    OnInit();
    return SUCCESS;
}

int LogicServer::OnInit()
{
    OnInitQueue();
    OnInitExchange();
    OnInitMsg();
    return SUCCESS;
}

int LogicServer::OnInitQueue()
{
    //查看queue文件夹是否存在
    string strQueueDir=DEFAULT_QUEUE_PATH;
    if(!FuncTool::IsFileExist(strQueueDir.c_str()))
    {
        return ERROR;
    }
    //遍历文件夹中的所有子文件夹
    DIR* pDir = opendir(strQueueDir.c_str());    
    if(!pDir)
    {
        return ERROR;
    }
    struct dirent *dir;
    struct stat st;
    char pBuffer[MAX_RDWR_FILE_BUFF_SIZE];
    while((dir = readdir(pDir)) != NULL)
    {
        if(strcmp(dir->d_name,".") == 0
                || strcmp(dir->d_name,"..") == 0)
        {
            continue;
        }    
        string sub_path = strQueueDir;
        sub_path=sub_path + "/" + dir->d_name;
        if(lstat(sub_path.c_str(),&st) == -1)
        {
            continue;
        }    
        if(S_ISDIR(st.st_mode))
        {
            //检查数据文件是否存在
            string strInfoFile=sub_path+"/info.bin";
            if(!FuncTool::IsFileExist(strInfoFile.c_str()))
            {
                continue;
            }
            //打开文件进行读取
            FILE *pQueueFile=fopen(strInfoFile.c_str(),"rb+");
            if(pQueueFile==NULL)
            {
                continue;
            }
            int iReadLen=MAX_NAME_LENGTH+sizeof(unsigned short)+sizeof(bool)+sizeof(bool);
            memset(pBuffer,0,MAX_RDWR_FILE_BUFF_SIZE);
            int iRet=fread(pBuffer,iReadLen,1,pQueueFile);
            fclose(pQueueFile);
            if(iRet==0)
            {
                continue;
            }
            //读名称
            char *pTemp=pBuffer;
            char pQueueName[MAX_NAME_LENGTH];
            memset(pQueueName,0,sizeof(pQueueName));
            int offset=FuncTool::ReadBuf(pTemp,pQueueName,sizeof(pQueueName));
            string strQueueName(pQueueName);
            //读优先级
            short iPriority;
            pTemp+=offset;
            offset=FuncTool::ReadShort(pTemp,iPriority);
            //读持久化信息
            bool bDurable=false;
            pTemp+=offset;
            offset=FuncTool::ReadBool(pTemp,bDurable);
            //读自动删除信息
            bool bAutoDel=true;
            pTemp+=offset;
            offset=FuncTool::ReadBool(pTemp,bAutoDel);
            MessageQueue *pMsgQueue=new MessageQueue(strQueueName,iPriority,bDurable,bAutoDel);

            //读持久化写位置文件
            string strOffsetFile=sub_path+"/write_offset.bin";
            FILE *pOffsetFile=fopen(strOffsetFile.c_str(),"rb+");
            if(pOffsetFile==NULL)
            {
                continue;
            }
            int iIndex=0;
            char pIndex[10];
            memset(pIndex,0,sizeof(pIndex));
            iRet=fread(pIndex,sizeof(int),1,pOffsetFile);
            FuncTool::ReadInt(pIndex,iIndex);
            if(iRet==0)
            {
                continue;
            }
            pMsgQueue->SetNextDaurableIndex(iIndex+1);
            fclose(pOffsetFile);

            //读订阅文件
            string strSubscribeFile=sub_path+"/subscribe.bin";
            if(!FuncTool::IsFileExist(strSubscribeFile.c_str()))
            {
                m_mUnSubcribeQueues[pQueueName]=pMsgQueue;
                continue;
            }
            pQueueFile=fopen(strSubscribeFile.c_str(),"rb+");
            if(pQueueFile==NULL)
            {
                m_mUnSubcribeQueues[pQueueName]=pMsgQueue;
                continue;
            }
            iReadLen=MAX_NAME_LENGTH+sizeof(unsigned char)+sizeof(unsigned short);
            memset(pBuffer,0,MAX_RDWR_FILE_BUFF_SIZE);
            iRet=fread(pBuffer,iReadLen,1,pQueueFile);
            if(iRet==0)
            {
                m_mUnSubcribeQueues[pQueueName]=pMsgQueue;
                continue;
            }
            //读名称
            pTemp=pBuffer;
            memset(pQueueName,0,sizeof(pQueueName));
            offset=FuncTool::ReadBuf(pTemp,pQueueName,sizeof(pQueueName));
            pTemp+=offset;
            //读确认级别
            unsigned char iConfirmLevel;
            offset=FuncTool::ReadByte(pTemp,iConfirmLevel);
            pTemp+=offset;
            pMsgQueue->SetConfirmLevel(iConfirmLevel);
            //读订阅数目
            unsigned short iSubscribeNum;
            offset=FuncTool::ReadShort(pTemp,iSubscribeNum);
            pTemp+=offset;
            if(iSubscribeNum==0)
            {
                m_mUnSubcribeQueues[pQueueName]=pMsgQueue;
                continue;
            }
            //读订阅者
            iReadLen=iSubscribeNum *sizeof(int);
            iRet=fread(pTemp,iReadLen,1,pQueueFile);
            if(iRet==0)
            {
                m_mUnSubcribeQueues[pQueueName]=pMsgQueue;
                continue;
            }
            for(int i=0;i<iSubscribeNum;++i)
            {
                int iIndex=0;
                offset=FuncTool::ReadInt(pTemp,iIndex);
                pMsgQueue->AddSubscribe(iIndex,iConfirmLevel);
            }
            m_mSubcribeQueues[strQueueName]=pMsgQueue;
            fclose(pQueueFile);

            
        }
        else
        {
            continue;
        }
    }
    closedir(pDir);
    return SUCCESS;
}

int LogicServer::OnInitExchange()
{
    //查看exchange文件夹是否存在
    string strExchangeDir=DEFAULT_EXCHANGE_PATH;
    if(!FuncTool::IsFileExist(strExchangeDir.c_str()))
    {
        return ERROR;
    }
    //遍历文件夹中的所有子文件夹
    DIR* pDir = opendir(strExchangeDir.c_str());    
    if(!pDir)
    {
        return ERROR;
    }
    struct dirent *dir;
    struct stat st;
    char pBuffer[MAX_RDWR_FILE_BUFF_SIZE];
    while((dir = readdir(pDir)) != NULL)
    {
        if(strcmp(dir->d_name,".") == 0
                || strcmp(dir->d_name,"..") == 0)
        {
            continue;
        }    
        string sub_path = strExchangeDir;
        sub_path=sub_path + "/" + dir->d_name;
        if(lstat(sub_path.c_str(),&st) == -1)
        {
            continue;
        }    
        if(S_ISDIR(st.st_mode))
        {
            //检查数据文件是否存在
            string strInfoFile=sub_path+"/info.bin";
            if(!FuncTool::IsFileExist(strInfoFile.c_str()))
            {
                continue;
            }
            //打开文件进行读取
            FILE *pExFile=fopen(strInfoFile.c_str(),"rb+");
            if(pExFile==NULL)
            {
                continue;
            }
            int iReadLen=MAX_NAME_LENGTH+sizeof(unsigned short)+sizeof(bool)+sizeof(bool);
            memset(pBuffer,0,MAX_RDWR_FILE_BUFF_SIZE);
            int iRet=fread(pBuffer,iReadLen,1,pExFile);
            fclose(pExFile);
            if(iRet==0)
            {
                continue;
            }
            //读名称
            char *pTemp=pBuffer;
            char pExchangeName[MAX_NAME_LENGTH];
            memset(pExchangeName,0,sizeof(pExchangeName));
            int offset=FuncTool::ReadBuf(pTemp,pExchangeName,sizeof(pExchangeName));
            string strExchangeName(pExchangeName);
            //读exchange 类型
            unsigned short iExchangeType;
            pTemp+=offset;
            offset=FuncTool::ReadShort(pTemp,iExchangeType);
            //读持久化信息
            bool bDurable=false;
            pTemp+=offset;
            offset=FuncTool::ReadBool(pTemp,bDurable);
            //读自动删除信息
            bool bAutoDel=true;
            pTemp+=offset;
            offset=FuncTool::ReadBool(pTemp,bAutoDel);
            Exchange *pExchange=new Exchange(strExchangeName,iExchangeType,bDurable,bAutoDel);
            m_mExchanges[strExchangeName]=pExchange;

            //读绑定文件
            string strBindingFile=sub_path+"/binding.bin";
            if(!FuncTool::IsFileExist(strBindingFile.c_str()))
            {
                continue;
            }
            pExFile=fopen(strBindingFile.c_str(),"rb+");
            if(pExFile==NULL)
            {
                continue;
            }
            iReadLen=MAX_NAME_LENGTH+sizeof(unsigned short);
            memset(pBuffer,0,MAX_RDWR_FILE_BUFF_SIZE);
            iRet=fread(pBuffer,iReadLen,1,pExFile);
            if(iRet==0)
            {
                continue;
            }
            //读名称
            pTemp=pBuffer;
            memset(pExchangeName,0,sizeof(pExchangeName));
            offset=FuncTool::ReadBuf(pTemp,pExchangeName,sizeof(pExchangeName));
            pTemp+=offset;
            //读绑定数目
            unsigned short iBindingNum;
            offset=FuncTool::ReadShort(pTemp,iBindingNum);
            pTemp+=offset;
            if(iBindingNum==0)
            {
                continue;
            }
            //读绑定关系
            char pBindingKey[MAX_NAME_LENGTH];
            char pQueueName[MAX_NAME_LENGTH];
            for(int i=0;i<iBindingNum;++i)
            {
                //读绑定key及对应绑定数目
                iReadLen=MAX_NAME_LENGTH+sizeof(unsigned short);
                iRet=fread(pTemp,iReadLen,1,pExFile);
                if(iRet==0)
                {
                    break;
                }
                memset(pBindingKey,0,sizeof(pBindingKey));
                offset=FuncTool::ReadBuf(pTemp,pBindingKey,sizeof(pBindingKey));
                pTemp+=offset;
                string strBindingKey=pBindingKey;
                unsigned short iSubBindingNum;
                offset=FuncTool::ReadShort(pTemp,iSubBindingNum);
                pTemp+=offset;
                //读每一个queue 名称
                iReadLen=MAX_NAME_LENGTH*iSubBindingNum;
                iRet=fread(pTemp,iReadLen,1,pExFile);
                if(iRet==0)
                {
                    break;
                }
                for(int j=0;j<iSubBindingNum;++j)
                {
                    memset(pQueueName,0,sizeof(pQueueName));
                    offset=FuncTool::ReadBuf(pTemp,pQueueName,sizeof(pQueueName));
                    pTemp+=offset;
                    string strQueueName=pQueueName;
                    MessageQueue *pMsgQueue=NULL;
                    if(m_mUnSubcribeQueues.find(strQueueName)!=m_mUnSubcribeQueues.end())
                    {
                        pMsgQueue=m_mUnSubcribeQueues[strQueueName];
                    }
                    if(m_mSubcribeQueues.find(strQueueName)!=m_mSubcribeQueues.end())
                    {
                        pMsgQueue=m_mSubcribeQueues[strQueueName];
                    }
                    if(pMsgQueue)
                    {
                        pExchange->CreateBinding(strBindingKey,pMsgQueue);
                    }
                }
            }
            fclose(pExFile);
        }
        else
        {
            continue;
        }
    }
    closedir(pDir);
    return SUCCESS;
}

int LogicServer::OnInitMsg()
{
    //查看queue文件夹是否存在
    string strQueueDir=DEFAULT_QUEUE_PATH;
    if(!FuncTool::IsFileExist(strQueueDir.c_str()))
    {
        return ERROR;
    }
    //遍历文件夹中的所有子文件夹
    DIR* pDir = opendir(strQueueDir.c_str());    
    if(!pDir)
    {
        return ERROR;
    }
    struct dirent *dir;
    struct stat st;
    char pBuffer[MAX_RDWR_FILE_BUFF_SIZE];
    while((dir = readdir(pDir)) != NULL)
    {
        if(strcmp(dir->d_name,".") == 0
                || strcmp(dir->d_name,"..") == 0)
        {
            continue;
        }    
        string sub_path = strQueueDir;
        sub_path=sub_path + "/" + dir->d_name;
        if(lstat(sub_path.c_str(),&st) == -1)
        {
            continue;
        }    
        if(S_ISDIR(st.st_mode))
        {
            //获取文件夹中所有的index文件名称
            vector<string>vIndexFiles;
            FindIndexFiles(sub_path.c_str(),vIndexFiles);
            //排序保证先处理最小文件
            std::sort(vIndexFiles.begin(),vIndexFiles.end());
            //遍历所有文件，依次进行处理
            for(int i=0;i<vIndexFiles.size();++i)
            {
                string strFile=sub_path+"/"+vIndexFiles[i];
                if(ProcessDurableMsgFile(strFile.c_str())==ERR_LOGIC_SERVER_QUEUE_NOT_EXIST)
                {
                    break;
                }
            }
        }
    }
    closedir(pDir);
    return SUCCESS;
}

string LogicServer::FindMessageFileName(int iMsgIndex,const char *ipPath)
{
    string strIndexName=ConvertIndexToString(iMsgIndex);
    string strRes="0000000000";
    //遍历文件夹中的所有子文件夹
    DIR* pDir = opendir(ipPath);    
    if(!pDir)
    {
        return "";
    }
    struct dirent *dir;
    struct stat st;
    char pBuffer[MAX_RDWR_FILE_BUFF_SIZE];
    while((dir = readdir(pDir)) != NULL)
    {
        if(strcmp(dir->d_name,".") == 0
                || strcmp(dir->d_name,"..") == 0)
        {
            continue;
        }    
        string sub_path = ipPath;
        sub_path=sub_path + "/" + dir->d_name;
        if(lstat(sub_path.c_str(),&st) == -1)
        {
            continue;
        } 
        string strName=dir->d_name;   
        if(S_ISREG(st.st_mode))
        {
            int iPos=strName.find(".index");
            if(iPos==string::npos)
            {
                continue;
            }
            strName=strName.substr(0,iPos);
            if(strName<=strIndexName&&strName>strRes)
            {
                strRes=strName;
            }
        }
    }
    closedir(pDir);
    return strRes;
}

string LogicServer::ConvertIndexToString(int iIndex)
{
    string strIndex=to_string(iIndex);
    int count=10-strIndex.size();
    if(count>0)
    {
        string strPrev(count,'0');
        strIndex=strPrev+strIndex;
    }
    return strIndex;
}

int LogicServer::FindPosInIndexFile(int iMsgIndex,const char *ipFile)
{
    int fd=open(ipFile,O_RDONLY);
    if(fd==-1)
    {
        return -1;
    }
    struct stat buf;
    if(fstat(fd,&buf)==-1||buf.st_size<=0)
    {
        return -1;
    }
    char *pStart=(char *)mmap(NULL,buf.st_size,PROT_READ,MAP_PRIVATE,fd,0);
    if(pStart==NULL)
    {
        return -1;
    }
    int lo=0;
    int hi=buf.st_size/8+1;
    int mid=0;
    int iIndex=0;
    while(lo<hi)
    {
        mid=lo+((hi-lo)>>1);
        iIndex=0;
        FuncTool::ReadInt(pStart+mid*8,iIndex);
        if(iMsgIndex==iIndex)
        {
            break;
        }
        else if(iMsgIndex>iIndex)
        {
            lo=mid+1;
        }
        else
        {
            hi=mid;
        }
    }
    if(iIndex!=iMsgIndex)
    {
        return -1;
    }
    mid=mid*8+sizeof(int);
    int iPos=0;
    FuncTool::ReadInt(pStart+mid,iPos);
    munmap(pStart,buf.st_size);
    return iPos;
}

int LogicServer::ProcessDataFile(const char *ipFile,int &oLastIndex,int iStartPos/*=0*/)
{
    int fd=open(ipFile,O_RDONLY);
    if(fd==-1)
    {
        return ERROR;
    }
    struct stat buf;
    if(fstat(fd,&buf)==-1||buf.st_size<=0)
    {
        return ERROR;
    }
    char *pStart=(char *)mmap(NULL,buf.st_size,PROT_READ,MAP_PRIVATE,fd,0);
    if(pStart==NULL)
    {
        return ERROR;
    }
    //从起始位置读数据
    char *pBuff=pStart+iStartPos;
    while(iStartPos<buf.st_size)
    {
        unsigned short iLen=0;
        int offset=FuncTool::ReadShort(pBuff,iLen);
        iStartPos+=iLen;
        pBuff+=offset;
        iLen-=offset;
        unsigned short iCmdId;
        offset=FuncTool::ReadShort(pBuff,iCmdId);
        pBuff+=offset;
        iLen-=offset;
        char pQueueName[MAX_NAME_LENGTH];
        memset(pQueueName,0,sizeof(pQueueName));
        offset=FuncTool::ReadBuf(pBuff,pQueueName,sizeof(pQueueName));
        string strQueueName=pQueueName;
        //查找队列是否存在
        MessageQueue *pMsgQueue=NULL;
        if(m_mUnSubcribeQueues.find(strQueueName)!=m_mUnSubcribeQueues.end())
        {
            pMsgQueue=m_mUnSubcribeQueues[strQueueName];
        }
        if(m_mSubcribeQueues.find(strQueueName)!=m_mSubcribeQueues.end())
        {
            pMsgQueue=m_mSubcribeQueues[strQueueName];
        }
        if(pMsgQueue==NULL)
        {
            return ERROR;
        }
        pBuff+=offset;
        iLen-=offset;
        int iDurableIndex=0;
        offset=FuncTool::ReadInt(pBuff,iDurableIndex);
        oLastIndex=iDurableIndex;
        pBuff+=offset;
        iLen-=offset;
        short iPriority=0;
        offset=FuncTool::ReadShort(pBuff,iPriority);
        pBuff+=offset;
        iLen-=offset;
        char pMsgBody[MAX_CLINT_PACKAGE_LENGTH];
        memset(pMsgBody,0,sizeof(pMsgBody));
        offset=FuncTool::ReadBuf(pBuff,pMsgBody,iLen);
        string strMsgBody=pMsgBody;
        pBuff+=offset;
        iLen-=offset;
        SeverStoreMessage *pMsg=new SeverStoreMessage(strMsgBody,iPriority,true);
        pMsg->SetDurableIndex(iDurableIndex);
        pMsgQueue->AddMessage(pMsg);
    }
    munmap(pStart,buf.st_size);
    return SUCCESS;
}

int LogicServer::FindIndexFiles(const char *ipDir,vector<string>&ovIndexFiles)
{
    //遍历文件夹中的所有子文件夹
    DIR* pDir = opendir(ipDir);    
    if(!pDir)
    {
        return ERROR;
    }
    struct dirent *dir;
    struct stat st;
    char pBuffer[MAX_RDWR_FILE_BUFF_SIZE];
    while((dir = readdir(pDir)) != NULL)
    {
        if(strcmp(dir->d_name,".") == 0
                || strcmp(dir->d_name,"..") == 0)
        {
            continue;
        }    
        string sub_path = ipDir;
        sub_path=sub_path + "/" + dir->d_name;
        if(lstat(sub_path.c_str(),&st) == -1)
        {
            continue;
        } 
        string strName=dir->d_name;
        if(S_ISREG(st.st_mode))
        {
            int iPos=strName.find(".index");
            if(iPos==string::npos)
            {
                continue;
            }
            strName=strName.substr(0,iPos);
            ovIndexFiles.push_back(strName);
        }
    }
    closedir(pDir);
    return SUCCESS;
}

int LogicServer::ProcessDurableMsgFile(const char *ipFile)
{
    //获取数据文件和索引文件是否存在
    string strFile=ipFile;
    string strIndexFile=strFile+".index";
    string strDataFile=strFile+".data";
    int indexFd=open(strIndexFile.c_str(),O_RDONLY);
    if(indexFd==-1)
    {
        return ERROR;
    }
    struct stat buf;
    if(fstat(indexFd,&buf)==-1||buf.st_size<=0)
    {
        return ERROR;
    }
    int indexFileSize=buf.st_size;

    int dataFd=open(strDataFile.c_str(),O_RDONLY);
    if(dataFd==-1)
    {
        return ERROR;
    }
    if(fstat(dataFd,&buf)==-1||buf.st_size<=0)
    {
        return ERROR;
    }
    int dataFileSize=buf.st_size;

    //将索引文件和数据文件映射到内存
    char *pIndexStart=(char *)mmap(NULL,indexFileSize,PROT_READ,MAP_PRIVATE,indexFd,0);
    if(pIndexStart==NULL)
    {
        return ERROR;
    }
    char *pDataStart=(char *)mmap(NULL,dataFileSize,PROT_READ,MAP_PRIVATE,dataFd,0);
    if(pDataStart==NULL)
    {
        munmap(pIndexStart,indexFileSize);
        return ERROR;
    }
    //将已经消费消息存入set
    string strConsumeFile=strFile+".consume";
    unordered_set<int>consumeIndexSet;
    int consumeFd=open(strConsumeFile.c_str(),O_RDONLY);
    if(consumeFd>0)
    {
        struct stat buf;
        if(fstat(consumeFd,&buf)==0&&buf.st_size>0)
        {
            int consumeFileSize=buf.st_size;
            char *pConsumeStart=(char *)mmap(NULL,consumeFileSize,PROT_READ,MAP_PRIVATE,consumeFd,0);
            if(pConsumeStart!=NULL)
            {
                int iPos=0;
                while(iPos<consumeFileSize)
                {
                    int iIndex=0;
                    int offset=FuncTool::ReadInt(pConsumeStart+iPos,iIndex);
                    consumeIndexSet.insert(iIndex);
                    iPos+=offset;
                }
                munmap(pConsumeStart,consumeFileSize);
            }
        }
    }
    //遍历所有消息，将未消费消息添加进队列
    int iPos=0;
    while(iPos<indexFileSize)
    {
        //读取消息下标及位置
        int iIndex=0;
        int offset=FuncTool::ReadInt(pIndexStart+iPos,iIndex);
        iPos+=offset;
        int iDataPos=0;
        offset=FuncTool::ReadInt(pIndexStart+iPos,iDataPos);
        iPos+=offset;
        //若消息已经被消费，则跳过
        if(consumeIndexSet.find(iIndex)!=consumeIndexSet.end())
        {
            continue;
        }
        //开始读取数据
        char *pBuff=pDataStart+iDataPos;
        unsigned short iLen=0;
        offset=FuncTool::ReadShort(pBuff,iLen);
        pBuff+=offset;
        iLen-=offset;
        unsigned short iCmdId;
        offset=FuncTool::ReadShort(pBuff,iCmdId);
        pBuff+=offset;
        iLen-=offset;
        char pQueueName[MAX_NAME_LENGTH];
        memset(pQueueName,0,sizeof(pQueueName));
        offset=FuncTool::ReadBuf(pBuff,pQueueName,sizeof(pQueueName));
        string strQueueName=pQueueName;
        //查找队列是否存在
        MessageQueue *pMsgQueue=NULL;
        if(m_mUnSubcribeQueues.find(strQueueName)!=m_mUnSubcribeQueues.end())
        {
            pMsgQueue=m_mUnSubcribeQueues[strQueueName];
        }
        if(m_mSubcribeQueues.find(strQueueName)!=m_mSubcribeQueues.end())
        {
            pMsgQueue=m_mSubcribeQueues[strQueueName];
        }
        if(pMsgQueue==NULL)
        {
            return ERR_LOGIC_SERVER_QUEUE_NOT_EXIST;
        }
        pBuff+=offset;
        iLen-=offset;
        int iDurableIndex=0;
        offset=FuncTool::ReadInt(pBuff,iDurableIndex);
        pBuff+=offset;
        iLen-=offset;
        short iPriority=0;
        offset=FuncTool::ReadShort(pBuff,iPriority);
        pBuff+=offset;
        iLen-=offset;
        char pMsgBody[MAX_CLINT_PACKAGE_LENGTH];
        memset(pMsgBody,0,sizeof(pMsgBody));
        offset=FuncTool::ReadBuf(pBuff,pMsgBody,iLen);
        string strMsgBody=pMsgBody;
        pBuff+=offset;
        iLen-=offset;
        SeverStoreMessage *pMsg=new SeverStoreMessage(strMsgBody,iPriority,true);
        pMsg->SetDurableIndex(iDurableIndex);
        pMsgQueue->AddMessage(pMsg);
    }
    munmap(pDataStart,dataFileSize);
    munmap(pIndexStart,indexFileSize);
    return SUCCESS;
}

int LogicServer::PushMessage()
{
    unordered_map<string,MessageQueue *>::iterator itr=m_mSubcribeQueues.begin();
    while (itr!=m_mSubcribeQueues.end())
    {
        MessageQueue *pMsgQueue=itr->second;
        ++itr;
        //检查一下是否订阅
        if(!pMsgQueue->IsSubscribe())
        {
            m_mSubcribeQueues.erase(itr++);
            m_mUnSubcribeQueues[pMsgQueue->GetQueueName()]=pMsgQueue;
            continue;
        }
        //推送消息
        while(!pMsgQueue->IsEmpty())
        {
            SeverStoreMessage *pMsg=pMsgQueue->PopFrontMsg(true);
            if(pMsg==NULL)
            {
                break;
            }
            char pBuffer[MAX_SERVER_PACKAGE_LENGTH];
            memset(pBuffer,0,sizeof(pBuffer));
            int iLen=MAX_SERVER_PACKAGE_LENGTH;
            pMsg->SetCmdId(CMD_SERVER_PUSH_MESSAGE);
            //若是需要确认，则消息放入待确认队列
            if(pMsgQueue->GetConfirmLevel()>0)
            {
                LogicServerLogger.WriteLog(mq_log_info,"push msg need ack");
                LogicServerLogger.Print(mq_log_info,"push msg need ack");
                pMsg->SetConfirmLevel(pMsgQueue->GetConfirmLevel());
                pMsg->SetMsgSeq(m_iAckSeq++);
                pMsg->SetBelongQueueName(pMsgQueue->GetQueueName());
                m_NeedAckMessageList.push_back(pMsg);
            }
            else
            {
                pMsg->SetMsgSeq(-1);
            }
            
            LogicServerLogger.WriteLog(mq_log_info,"find a msg,msg seq is %d,msg body is %s",pMsg->GetMsgSeq(),pMsg->GetMsgBody().c_str());
            LogicServerLogger.Print(mq_log_info,"find a msg,msg seq is %d,msg body is %s",pMsg->GetMsgSeq(),pMsg->GetMsgBody().c_str());
            
            pMsg->GetMessagePack(pBuffer,&iLen);
            LogicServerLogger.WriteLog(mq_log_info,"push msg into shmqueue...,msg is %s",pBuffer);
            LogicServerLogger.Print(mq_log_info,"push msg into shmqueue...msg is %s",pBuffer);
            //LOG_INFO(0, 0, "push one msg : %s",pBuffer);
            while(m_pQueueToConnect->Enqueue(pBuffer,iLen)==ShmQueue::ERR_SHM_QUEUE_FULL)
            {
                //LOG_ERROR(0, 0, "push msg to consrv failed");
            }
            ++m_iSendCount;
            if(m_iSendCount%10000==0)
            {
                //LOG_INFO(0, 0, "send the %d msg %s",m_iSendCount,pBuffer+12);
            }
            if(pMsgQueue->GetConfirmLevel()==0)
            {
                //若消息是持久化的，则向持久化层发包
                if(pMsg->IsDurable())
                {
                    string strQueueName=pMsgQueue->GetQueueName();
                    char pBuff[MAX_CLINT_PACKAGE_LENGTH];
                    memset(pBuff,0,sizeof(pBuff));
                    unsigned short iPackLen=sizeof(unsigned short)+sizeof(unsigned short)+MAX_NAME_LENGTH+sizeof(int);
                    char *pTemp=pBuff;
                    int offset=FuncTool::WriteShort(pTemp,iPackLen);
                    pTemp+=offset;
                    offset=FuncTool::WriteShort(pTemp,CMD_SERVER_PUSH_MESSAGE);
                    pTemp+=offset;
                    offset=FuncTool::WriteBuf(pTemp,strQueueName.c_str(),strQueueName.size());
                    pTemp+=MAX_NAME_LENGTH;
                    offset=FuncTool::WriteInt(pTemp,pMsg->GetDurableIndex());
                    m_pQueueToPersis->Enqueue(pBuff,iPackLen);
                }
                delete pMsg;
                pMsg=NULL;
            }
        }
    }
    return SUCCESS;
}

int LogicServer::OnCreateExchange(char *ipBuffer,int iLen,string &ostrMsgBody)
{
    char *pTemp=ipBuffer;
    //读名称
    char pExchangeName[MAX_NAME_LENGTH];
    memset(pExchangeName,0,sizeof(pExchangeName));
    int offset=FuncTool::ReadBuf(pTemp,pExchangeName,sizeof(pExchangeName));
    string strExchangeName(pExchangeName);
    //读exchange 类型
    unsigned short iExchangeType;
    pTemp+=offset;
    offset=FuncTool::ReadShort(pTemp,iExchangeType);
    //读持久化信息
    bool bDurable=false;
    pTemp+=offset;
    offset=FuncTool::ReadBool(pTemp,bDurable);
    //读自动删除信息
    bool bAutoDel=true;
    pTemp+=offset;
    offset=FuncTool::ReadBool(pTemp,bAutoDel);
    LogicServerLogger.WriteLog(mq_log_info,"begain create exchange,name is %s",pExchangeName);
    LogicServerLogger.Print(mq_log_info,"begain create exchange,name is %s",pExchangeName);
    //看是否存在同名exchange
    if(m_mExchanges.find(strExchangeName)!=m_mExchanges.end())
    {
        ostrMsgBody=RE_EXCHANGE_EXIST+strExchangeName;
        LogicServerLogger.WriteLog(mq_log_info,"exchange is exist,name is %s",pExchangeName);
        LogicServerLogger.Print(mq_log_info,"exchange is exist,name is %s",pExchangeName);
        return SUCCESS;
    }
    else
    {
        Exchange *pExchange=new Exchange(strExchangeName,iExchangeType,bDurable,bAutoDel);
        if(pExchange==NULL)
        {
            LogicServerLogger.WriteLog(mq_log_err,"create exchange failed,name is %s",pExchangeName);
            LogicServerLogger.Print(mq_log_err,"create exchange failed,name is %s",pExchangeName);
            ostrMsgBody=RE_CREATE_EXCHANGE_FAILED;
            return ERR_LOGIC_SERVER_CREATE_EXCHANGE;
        }
        else
        {
            LogicServerLogger.WriteLog(mq_log_info,"create exchange succeed,name is %s",pExchangeName);
            LogicServerLogger.Print(mq_log_info,"create exchange succeed,name is %s",pExchangeName);
            m_mExchanges[strExchangeName]=pExchange;
            //若是需要持久化的exchange,则重新组包发送给持久化层
            if(bDurable)
            {
                char pBuff[MAX_CLINT_PACKAGE_LENGTH];
                memset(pBuff,0,sizeof(pBuff));
                unsigned short iPackLen=sizeof(unsigned short)+sizeof(unsigned short)+iLen;
                pTemp=pBuff;
                offset=FuncTool::WriteShort(pTemp,iPackLen);
                pTemp+=offset;
                offset=FuncTool::WriteShort(pTemp,CMD_CREATE_EXCNANGE);
                pTemp+=offset;
                offset=FuncTool::WriteBuf(pTemp,ipBuffer,iLen);
                if(m_pQueueToPersis->Enqueue(pBuff,iPackLen)!=SUCCESS)
                {
                    ostrMsgBody=RE_DURABLE_FAILED;
                    return ERR_LOGIC_SERVER_CREATE_EXCHANGE;
                }
                LogicServerLogger.WriteLog(mq_log_info,"send durable exchange info to persis,name is %s");
                LogicServerLogger.Print(mq_log_info,"send durable exchange info to persis");
            }
            ostrMsgBody=RE_CREATE_EXCHANGE_SUCCESS;
            return SUCCESS;
        }
    }
}

int LogicServer::OnCreateQueue(char *ipBuffer,int iLen,string &ostrMsgBody)
{
    char *pTemp=ipBuffer;
    //读名称
    char pQueueName[MAX_NAME_LENGTH];
    memset(pQueueName,0,sizeof(pQueueName));
    int offset=FuncTool::ReadBuf(pTemp,pQueueName,sizeof(pQueueName));
    string strQueueName(pQueueName);
    //读优先级
    short iPriority;
    pTemp+=offset;
    offset=FuncTool::ReadShort(pTemp,iPriority);
    //读持久化信息
    bool bDurable=false;
    pTemp+=offset;
    offset=FuncTool::ReadBool(pTemp,bDurable);
    //读自动删除信息
    bool bAutoDel=true;
    pTemp+=offset;
    offset=FuncTool::ReadBool(pTemp,bAutoDel);
    LogicServerLogger.WriteLog(mq_log_info,"begain create queue,name is %s",pQueueName);
    LogicServerLogger.Print(mq_log_info,"begain create queue,name is %s",pQueueName);
    //看是否存在同名queue
    if(m_mUnSubcribeQueues.find(strQueueName)!=m_mUnSubcribeQueues.end()||
    m_mSubcribeQueues.find(strQueueName)!=m_mSubcribeQueues.end())
    {
        LogicServerLogger.WriteLog(mq_log_info,"queue is exist,name is %s",pQueueName);
        LogicServerLogger.Print(mq_log_info,"queue is exist,name is %s",pQueueName);
        ostrMsgBody=RE_QUEUE_EXIST+strQueueName;
        return SUCCESS;
    }
    else 
    {
        MessageQueue *pMessageQueue=new MessageQueue(strQueueName,iPriority,bDurable,bAutoDel);
        if(pMessageQueue==NULL)
        {
            LogicServerLogger.WriteLog(mq_log_err,"create queue failed,name is %s",pQueueName);
            LogicServerLogger.Print(mq_log_err,"create queue failed,name is %s",pQueueName);
            ostrMsgBody=RE_CREATE_QUEUE_FAILED;
            return ERR_LOGIC_SERVER_CREATE_QUEUE;
        }
        else
        {
            LogicServerLogger.WriteLog(mq_log_info,"create queue succeed,name is %s",pQueueName);
            LogicServerLogger.Print(mq_log_info,"create queue succeed,name is %s",pQueueName);
            m_mUnSubcribeQueues[strQueueName]=pMessageQueue;
            //若是需要持久化的exchange,则重新组包发送给持久化层
            if(bDurable)
            {
                char pBuff[MAX_CLINT_PACKAGE_LENGTH];
                memset(pBuff,0,sizeof(pBuff));
                unsigned short iPackLen=sizeof(unsigned short)+sizeof(unsigned short)+iLen;
                pTemp=pBuff;
                offset=FuncTool::WriteShort(pTemp,iPackLen);
                pTemp+=offset;
                offset=FuncTool::WriteShort(pTemp,CMD_CREATE_QUEUE);
                pTemp+=offset;
                offset=FuncTool::WriteBuf(pTemp,ipBuffer,iLen);
                if(m_pQueueToPersis->Enqueue(pBuff,iPackLen)!=SUCCESS)
                {
                    ostrMsgBody=RE_DURABLE_FAILED;
                    return ERR_LOGIC_SERVER_CREATE_EXCHANGE;
                }
            }
            ostrMsgBody=RE_CREATE_QUEUE_SUCCESS;
            return SUCCESS;
        }
    }
}

int LogicServer::OnCreateBinding(char *ipBuffer,int iLen,string &ostrMsgBody)
{
    char *pTemp=ipBuffer;
    //读exchange名称
    char pExchangeName[MAX_NAME_LENGTH];
    memset(pExchangeName,0,sizeof(pExchangeName));
    int offset=FuncTool::ReadBuf(pTemp,pExchangeName,sizeof(pExchangeName));
    pTemp+=offset;
    string strExchangeName=pExchangeName;
    //读取queue名称
    char pQueueName[MAX_NAME_LENGTH];
    memset(pQueueName,0,sizeof(pQueueName));
    offset=FuncTool::ReadBuf(pTemp,pQueueName,sizeof(pQueueName));
    pTemp+=offset;
    string strQueueName=pQueueName;
    //读取binding key
    char pBindingKey[MAX_NAME_LENGTH];
    memset(pBindingKey,0,sizeof(pBindingKey));
    offset=FuncTool::ReadBuf(pTemp,pBindingKey,sizeof(pBindingKey));
    pTemp+=offset;
    string strBindingKey(pBindingKey);
    LogicServerLogger.WriteLog(mq_log_info,"begain create binding,name is %s|%s|%s",pExchangeName,pQueueName,pBindingKey);
    LogicServerLogger.Print(mq_log_info,"begain create binding,name is %s|%s|%s",pExchangeName,pQueueName,pBindingKey);
    //查询指定的exchange与queue是否存在
    Exchange *pExchange=NULL;
    MessageQueue *pMessageQueue=NULL;
    if(m_mExchanges.find(strExchangeName)!=m_mExchanges.end())
    {
        pExchange=m_mExchanges[strExchangeName];
    }
    if(m_mUnSubcribeQueues.find(strQueueName)!=m_mUnSubcribeQueues.end())
    {
        pMessageQueue=m_mUnSubcribeQueues[strQueueName];
    }
    else if(m_mSubcribeQueues.find(strQueueName)!=m_mSubcribeQueues.end())
    {
        pMessageQueue=m_mSubcribeQueues[strQueueName];
    }
    if(pExchange==NULL)
    {
        LogicServerLogger.WriteLog(mq_log_err,"exchange isn't exist,name is %s",pExchangeName);
        LogicServerLogger.Print(mq_log_err,"exchange isn't exist,name is %s",pExchangeName);
        ostrMsgBody=RE_EXCHANGE_NOT_EXIST+strExchangeName;
        return ERR_LOGIC_SERVER_CREATE_BINDING;
    }
    if(pMessageQueue==NULL)
    {
        LogicServerLogger.WriteLog(mq_log_err,"queue isn't exist,name is %s",pQueueName);
        LogicServerLogger.Print(mq_log_err,"queue isn't exist,name is %s",pQueueName);
        ostrMsgBody=RE_QUEUE_NOT_EXIST+strQueueName;
        return ERR_LOGIC_SERVER_CREATE_BINDING;
    }
    pExchange->CreateBinding(strBindingKey,pMessageQueue);
    //若是需要持久化的exchange则将当前绑定者发送给持久化层
    if(pExchange->IsDurable())
    {
        char pBuff[MAX_CLINT_PACKAGE_LENGTH];
        memset(pBuff,0,sizeof(pBuff));
        int iLen=MAX_CLINT_PACKAGE_LENGTH;
        pExchange->SerializeBindingToString(pBuff,iLen);
        if(m_pQueueToPersis->Enqueue(pBuff,iLen)!=SUCCESS)
        {
            ostrMsgBody=RE_BINDING_FAILED+strBindingKey;
            return ERR_LOGIC_SERVER_CREATE_BINDING;
        }
    }
    ostrMsgBody=RE_BINDING_SUCCEED+strBindingKey;
    LogicServerLogger.WriteLog(mq_log_info,"create binding succeed");
    LogicServerLogger.Print(mq_log_info,"create binding succeed");
    return SUCCESS;
}

int LogicServer::OnPublishMessage(char *ipBuffer,int iLen,int iCliIndex,string &ostrMsgBody)
{
    char *pTemp=ipBuffer;
    //读exchange名称
    char pExchangeName[MAX_NAME_LENGTH];
    memset(pExchangeName,0,sizeof(pExchangeName));
    int offset=FuncTool::ReadBuf(pTemp,pExchangeName,sizeof(pExchangeName));
    pTemp+=offset;
    iLen-=offset;
    string strExchangeName=pExchangeName;
    //读取routing key
    char pRoutingKey[MAX_NAME_LENGTH];
    memset(pRoutingKey,0,sizeof(pRoutingKey));
    offset=FuncTool::ReadBuf(pTemp,pRoutingKey,sizeof(pRoutingKey));
    pTemp+=offset;
    iLen-=offset;
    string strRoutingKey=pRoutingKey;
    //读优先级
    unsigned short iPriority;
    offset=FuncTool::ReadShort(pTemp,iPriority);
    //读持久化信息
    bool bDurable=false;
    pTemp+=offset;
    iLen-=offset;
    offset=FuncTool::ReadBool(pTemp,bDurable);
    //读序号
    int iMsgSeq=0;
    pTemp+=offset;
    iLen-=offset;
    offset=FuncTool::ReadInt(pTemp,iMsgSeq);
    string strMsgSeq=to_string(iCliIndex)+"#"+to_string(iMsgSeq);
    //读确认级别
    unsigned char confirmLevel;
    pTemp+=offset;
    iLen-=offset;
    offset=FuncTool::ReadByte(pTemp,confirmLevel);
    //检查是否是未确认的重复消息
    if(confirmLevel>0&&m_producerMsgSeqs.find(strMsgSeq)!=m_producerMsgSeqs.end())
    {
        ServerAckMessage oMsg(iMsgSeq);
        char pAckPack[MAX_SERVER_PACKAGE_LENGTH];
        memset(pAckPack,0,sizeof(pAckPack));
        int iLen=MAX_SERVER_PACKAGE_LENGTH;
        oMsg.GetMessagePack(pAckPack,&iLen);
        //确认包入队
        m_pQueueToConnect->Enqueue(pAckPack,iLen);
        LogicServerLogger.WriteLog(mq_log_info,"recv repeat msg,msg seq is %s",strMsgSeq.c_str());
        LogicServerLogger.Print(mq_log_info,"recv repeat msg,msg seq is %s",strMsgSeq.c_str());
        return SUCCESS;
    }
    LogicServerLogger.WriteLog(mq_log_info,"begain publish msg,name is %s|%s",pExchangeName,pRoutingKey);
    LogicServerLogger.Print(mq_log_info,"begain publish msg,name is %s|%s",pExchangeName,pRoutingKey);
    //检查exchange是否存在
    Exchange *pExchange=NULL;
    if(m_mExchanges.find(strExchangeName)!=m_mExchanges.end())
    {
        pExchange=m_mExchanges[strExchangeName];
    }
    if(pExchange==NULL)
    {
        LogicServerLogger.WriteLog(mq_log_err,"exchange isn't exist,name is %s",pExchangeName);
        LogicServerLogger.Print(mq_log_err,"exchange isn't exist,name is %s",pExchangeName);
        ostrMsgBody=RE_EXCHANGE_NOT_EXIST+strExchangeName;
        return ERR_LOGIC_SERVER_PUBLISH_MESSAGE;
    }
    //查找绑定队列
    list<MessageQueue *>msgQueueList;
    pExchange->FindBindingQueue(strRoutingKey,msgQueueList);
    if(msgQueueList.empty())
    {
        LogicServerLogger.WriteLog(mq_log_err,"fit queue isn't exist");
        LogicServerLogger.Print(mq_log_err,"fit queue isn't exist");
        ostrMsgBody=RE_FIT_QUEUE_NOT_EXIST+strRoutingKey;
        return ERR_LOGIC_SERVER_PUBLISH_MESSAGE;
    }
    LogicServerLogger.WriteLog(mq_log_info,"exchange binding queue num is %d",msgQueueList.size());
    LogicServerLogger.Print(mq_log_info,"exchange binding queue num is %d",msgQueueList.size());
    
    //读消息体
    pTemp+=offset;
    iLen-=offset;
    string strMsgBody;
    strMsgBody.assign(pTemp,iLen);
    list<MessageQueue *>::iterator itr=msgQueueList.begin();
    while(itr!=msgQueueList.end())
    {
        MessageQueue *pMsgQueue=*itr;
        SeverStoreMessage *pMsg=new SeverStoreMessage(strMsgBody,iPriority,bDurable);
        //若需要持久化，则设置持久化下标,并发包给持久化层
        if(bDurable&&pMsgQueue->IsDurable())
        {
            int iDurableIndex=pMsgQueue->GetNextDaurableIndex();
            pMsg->SetDurableIndex(iDurableIndex);
            char pBuff[MAX_CLINT_PACKAGE_LENGTH];
            memset(pBuff,0,sizeof(pBuff));
            int iPackLen=MAX_CLINT_PACKAGE_LENGTH;
            pMsg->SetBelongQueueName(pMsgQueue->GetQueueName());
            pMsg->SerializeDurableToString(pBuff,iPackLen);
            m_pQueueToPersis->Enqueue(pBuff,iPackLen);
        }
        pMsgQueue->AddMessage(pMsg);
        LogicServerLogger.WriteLog(mq_log_info,"put msg: %s into queue!",strMsgBody.c_str());
        LogicServerLogger.Print(mq_log_info,"put msg: %s into queue!",strMsgBody.c_str());
        ++itr;
        // //检查一下队列是否是被订阅，若有订阅，则直接推送消息
        // if(!pMsgQueue->IsSubscribe())
        // {
        //     continue;
        // }
        // //推送消息
        // while(!pMsgQueue->IsEmpty())
        // {
        //     SeverStoreMessage *pMsg=pMsgQueue->PopFrontMsg(true);
        //     if(pMsg==NULL)
        //     {
        //         break;
        //     }
        //     char pBuffer[MAX_SERVER_PACKAGE_LENGTH];
        //     int iLen=MAX_SERVER_PACKAGE_LENGTH;
        //     pMsg->SetCmdId(CMD_SERVER_PUSH_MESSAGE);
        //     //若是需要确认，则消息放入待确认队列
        //     if(pMsgQueue->GetConfirmLevel()>0)
        //     {
        //         LogicServerLogger.WriteLog(mq_log_info,"push msg need ack");
        //         LogicServerLogger.Print(mq_log_info,"push msg need ack");
        //         pMsg->SetMsgSeq(m_iAckSeq++);
        //         pMsg->SetBelongQueueName(pMsgQueue->GetQueueName());
        //         m_NeedAckMessageList.push_back(pMsg);
        //     }
        //     else
        //     {
        //         pMsg->SetMsgSeq(-1);
        //     }
            
        //     LogicServerLogger.WriteLog(mq_log_info,"find a msg,msg seq is %d,msg body is %s",pMsg->GetMsgSeq(),pMsg->GetMsgBody().c_str());
        //     LogicServerLogger.Print(mq_log_info,"find a msg,msg seq is %d,msg body is %s",pMsg->GetMsgSeq(),pMsg->GetMsgBody().c_str());
            
        //     pMsg->GetMessagePack(pBuffer,&iLen);
        //     LogicServerLogger.WriteLog(mq_log_info,"push msg into shmqueue...,msg is %s",pBuffer);
        //     LogicServerLogger.Print(mq_log_info,"push msg into shmqueue...msg is %s",pBuffer);
        //     LOG_INFO(0, 0, "push one msg : %s",pBuffer);
        //     m_pQueueToConnect->Enqueue(pBuffer,iLen);
        //     if(pMsgQueue->GetConfirmLevel()==0)
        //     {
        //         //若消息是持久化的，则向持久化层发包
        //         if(pMsg->IsDurable())
        //         {
        //             string strQueueName=pMsgQueue->GetQueueName();
        //             char pBuff[MAX_CLINT_PACKAGE_LENGTH];
        //             unsigned short iPackLen=sizeof(unsigned short)+sizeof(unsigned short)+MAX_NAME_LENGTH+sizeof(int);
        //             char *pTemp=pBuff;
        //             int offset=FuncTool::WriteShort(pTemp,iPackLen);
        //             pTemp+=offset;
        //             offset=FuncTool::WriteShort(pTemp,CMD_SERVER_PUSH_MESSAGE);
        //             pTemp+=offset;
        //             offset=FuncTool::WriteBuf(pTemp,strQueueName.c_str(),strQueueName.size());
        //             pTemp+=MAX_NAME_LENGTH;
        //             offset=FuncTool::WriteInt(pTemp,pMsg->GetDurableIndex());
        //             m_pQueueToPersis->Enqueue(pBuff,iPackLen);
        //         }
        //         delete pMsg;
        //         pMsg=NULL;
        //     }
        // }
    }
     //若是单个确认,直接确认
    if(confirmLevel==PRODUCER_SINGLE_ACK)
    {
        LogicServerLogger.WriteLog(mq_log_info,"the msg need ack");
        LogicServerLogger.Print(mq_log_info,"the msg need ack");
        ServerAckMessage oMsg(iMsgSeq);
        oMsg.SetClientIndex(iCliIndex);
        char pAckPack[MAX_SERVER_PACKAGE_LENGTH];
        memset(pAckPack,0,sizeof(pAckPack));
        int iLen=MAX_SERVER_PACKAGE_LENGTH;
        oMsg.GetMessagePack(pAckPack,&iLen);
        //确认包入队
        m_pQueueToConnect->Enqueue(pAckPack,iLen);
        //保存收到消息
        m_producerMsgSeqs.insert(strMsgSeq);
    }   
    //若是批量确认，则保存最新序号
    if(confirmLevel==PRODUCER_MUTIL_ACK)
    {
        m_producerLastMsgSeq[iCliIndex]=iMsgSeq;
        //保存收到消息
        m_producerMsgSeqs.insert(strMsgSeq);
    }
    return SUCCESS;
}

int LogicServer::SendMultiAck()
{
    unordered_map<int,int>::iterator itr=m_producerLastMsgSeq.begin();
    while(itr!=m_producerLastMsgSeq.end())
    {
        ServerAckMessage oMsg(itr->second);
        oMsg.SetClientIndex(itr->first);
        char pAckPack[MAX_SERVER_PACKAGE_LENGTH];
        memset(pAckPack,0,sizeof(pAckPack));
        int iLen=MAX_SERVER_PACKAGE_LENGTH;
        oMsg.GetMessagePack(pAckPack,&iLen);
        //确认包入队
        m_pQueueToConnect->Enqueue(pAckPack,iLen);
    }
    //清空集合
    m_producerLastMsgSeq.clear();
}

int LogicServer::OnRecvMessage(char *ipBuffer,int iLen,int iCliIndex,string &ostrMsgBody)
{
    char *pTemp=ipBuffer;
    //读取queue名称
    char pQueueName[MAX_NAME_LENGTH];
    memset(pQueueName,0,sizeof(pQueueName));
    int offset=FuncTool::ReadBuf(pTemp,pQueueName,sizeof(pQueueName));
    string strQueueName=pQueueName;

    //读确认级别
    unsigned char confirmLevel;
    pTemp+=offset;
    offset=FuncTool::ReadByte(pTemp,confirmLevel);
    LogicServerLogger.WriteLog(mq_log_info,"begain recv msg,queue name is %s",pQueueName);
    LogicServerLogger.Print(mq_log_info,"begain recv msg,queue name is %s",pQueueName);

    //检查queue是否存在
    MessageQueue* pMsgQueue=NULL;
    if(m_mUnSubcribeQueues.find(strQueueName)!=m_mUnSubcribeQueues.end())
    {
        pMsgQueue=m_mUnSubcribeQueues[strQueueName];
    }
    if(m_mSubcribeQueues.find(strQueueName)!=m_mSubcribeQueues.end())
    {
        pMsgQueue=m_mSubcribeQueues[strQueueName];
    }
    if(pMsgQueue==NULL)
    {
        LogicServerLogger.WriteLog(mq_log_err,"queue isn't exist,name is %s",pQueueName);
        LogicServerLogger.Print(mq_log_err,"queue isn't exist,name is %s",pQueueName);
        ostrMsgBody=RE_QUEUE_NOT_EXIST+strQueueName;
        return ERR_LOGIC_SERVER_RECV_MESSAGE;
    }
    SeverStoreMessage*pMsg=pMsgQueue->PopFrontMsg(false);
    if(pMsg==NULL)
    {
        LogicServerLogger.WriteLog(mq_log_err,"queue is empty,name is %s",pQueueName);
        LogicServerLogger.Print(mq_log_err,"queue is empty,name is %s",pQueueName);
        ostrMsgBody=RE_QUEUE_IS_EMPTY+strQueueName;
        return ERR_LOGIC_SERVER_RECV_MESSAGE;
    }
    pMsg->SetClientIndex(iCliIndex);
    //若需要确认，则设置消息确认号
    if(confirmLevel>0)
    {
        LogicServerLogger.WriteLog(mq_log_info,"msg need ack,seq is %d",m_iAckSeq);
        LogicServerLogger.Print(mq_log_info,"msg need ack,seq is %d",m_iAckSeq);
        pMsg->SetConfirmLevel(confirmLevel);
        pMsg->SetMsgSeq(m_iAckSeq++);
        pMsg->SetBelongQueueName(strQueueName);
    }
    else
    {
        pMsg->SetMsgSeq(-1);
    }
    char pBuffer[MAX_CLINT_PACKAGE_LENGTH];
    iLen=MAX_CLINT_PACKAGE_LENGTH;
    pMsg->GetMessagePack(pBuffer,&iLen);
    if(confirmLevel>0)
    {
        pMsg->SetSaveTime();
        m_NeedAckMessageList.push_back(pMsg);
    }
    else
    {
        //若消息是持久化的，则向持久化层发包
        if(pMsg->IsDurable())
        {
            char pBuff[MAX_CLINT_PACKAGE_LENGTH];
            memset(pBuff,0,sizeof(pBuff));
            unsigned short iPackLen=sizeof(unsigned short)+sizeof(unsigned short)+MAX_NAME_LENGTH+sizeof(int);
            pTemp=pBuff;
            offset=FuncTool::WriteShort(pTemp,iPackLen);
            pTemp+=offset;
            offset=FuncTool::WriteShort(pTemp,CMD_CREATE_RECV);
            pTemp+=offset;
            offset=FuncTool::WriteBuf(pTemp,strQueueName.c_str(),strQueueName.size());
            pTemp+=MAX_NAME_LENGTH;
            offset=FuncTool::WriteInt(pTemp,pMsg->GetDurableIndex());
            m_pQueueToPersis->Enqueue(pBuff,iPackLen);
        }
        delete pMsg;
        pMsg=NULL;
    }
    //消息放入队列
    while(m_pQueueToConnect->Enqueue(pBuffer,iLen)==ShmQueue::ERR_SHM_QUEUE_FULL)
    {
        //LOG_ERROR(0, 0, "push msg to consrv failed");
    }
    LogicServerLogger.WriteLog(mq_log_info,"recv msg succeed,msg put into shmqueue");
    LogicServerLogger.Print(mq_log_info,"recv msg succeed,msg put into shmqueue");
    return SUCCESS;
}

int LogicServer::OnCreateSubscribe(char *ipBuffer,int iLen,int iCliIndex,string &ostrMsgBody)
{
    char *pTemp=ipBuffer;
    //读取queue名称
    char pQueueName[MAX_NAME_LENGTH];
    memset(pQueueName,0,sizeof(pQueueName));
    int offset=FuncTool::ReadBuf(pTemp,pQueueName,sizeof(pQueueName));
    string strQueueName=pQueueName;

    //读确认级别
    unsigned char confirmLevel;
    pTemp+=offset;
    offset=FuncTool::ReadByte(pTemp,confirmLevel);

    LogicServerLogger.WriteLog(mq_log_info,"begain create subscribe,queue name is %s",pQueueName);
    LogicServerLogger.Print(mq_log_info,"begain create subscribe,queue name is %s",pQueueName);

    //检查queue是否存在
    MessageQueue* pMsgQueue=NULL;
    bool bIsSub=true;
    if(m_mUnSubcribeQueues.find(strQueueName)!=m_mUnSubcribeQueues.end())
    {
        pMsgQueue=m_mUnSubcribeQueues[strQueueName];
        bIsSub=false;
    }
    if(m_mSubcribeQueues.find(strQueueName)!=m_mSubcribeQueues.end())
    {
        pMsgQueue=m_mSubcribeQueues[strQueueName];
    }
    if(pMsgQueue==NULL)
    {
        LogicServerLogger.WriteLog(mq_log_err,"queue isn't exist,name is %s",pQueueName);
        LogicServerLogger.Print(mq_log_err,"queue isn't exist,name is %s",pQueueName);
        ostrMsgBody=RE_QUEUE_NOT_EXIST+strQueueName;
        return ERR_LOGIC_SERVER_CREATE_SUBSCRIBE;
    }
    pMsgQueue->AddSubscribe(iCliIndex,confirmLevel);
    //若是需要持久化的queue,则将当前订阅消费者发送给用户
    if(pMsgQueue->IsDurable())
    {
        char pBuff[MAX_CLINT_PACKAGE_LENGTH];
        memset(pBuff,0,sizeof(pBuff));
        int iLen=MAX_CLINT_PACKAGE_LENGTH;
        pMsgQueue->SerializeSubcribeToString(pBuff,iLen,true);
        if(m_pQueueToPersis->Enqueue(pBuff,iLen)!=SUCCESS)
        {
            pMsgQueue->CancelSubscribe(iCliIndex);
            ostrMsgBody=RE_SUBSCRIBE_FAILED+strQueueName;
            return ERR_LOGIC_SERVER_CREATE_SUBSCRIBE;
        }
    }
    //若原先未订阅，增加订阅
    if(!bIsSub)
    {
        m_mUnSubcribeQueues.erase(strQueueName);
        m_mSubcribeQueues[strQueueName]=pMsgQueue;
    }
    ostrMsgBody=RE_SUBSCRIBE_SUCCEED+strQueueName;
    LogicServerLogger.WriteLog(mq_log_info,"create subscribe succeed");
    LogicServerLogger.Print(mq_log_info,"create subscribe succeed");
    return SUCCESS;
}


int LogicServer::OnDeleteExchange(char *ipBuffer,int iLen,string &ostrMsgBody)
{
    char *pTemp=ipBuffer;
    //读名称
    char pExchangeName[MAX_NAME_LENGTH];
    memset(pExchangeName,0,sizeof(pExchangeName));
    FuncTool::ReadBuf(pTemp,pExchangeName,sizeof(pExchangeName));
    string strExchangeName=pExchangeName;
    LogicServerLogger.WriteLog(mq_log_info,"begain delete exchange,exchange name is %s",pExchangeName);
    LogicServerLogger.Print(mq_log_info,"begain delete exchange,exchange name is %s",pExchangeName);
    //看是否存在同名exchange
    if(m_mExchanges.find(strExchangeName)==m_mExchanges.end())
    {
        LogicServerLogger.WriteLog(mq_log_err,"exchange isn't exist,name is %s",pExchangeName);
        LogicServerLogger.Print(mq_log_err,"exchange isn't exist,name is %s",pExchangeName);
        ostrMsgBody=RE_EXCHANGE_NOT_EXIST+strExchangeName;
        return ERR_LOGIC_SERVER_DELETE_EXCHANGE;
    }
    Exchange *pExchange=m_mExchanges[strExchangeName];
    //若是需要持久化的exchange，则发送删除消息
    if(pExchange->IsDurable())
    {
        char pBuff[MAX_CLINT_PACKAGE_LENGTH];
        memset(pBuff,0,sizeof(pBuff));
        unsigned short iPackLen=sizeof(unsigned short)+sizeof(unsigned short)+iLen;
        pTemp=pBuff;
        int offset=FuncTool::WriteShort(pTemp,iPackLen);
        pTemp+=offset;
        offset=FuncTool::WriteShort(pTemp,CMD_DELETE_EXCHANGE);
        pTemp+=offset;
        offset=FuncTool::WriteBuf(pTemp,ipBuffer,iLen);
        if(m_pQueueToPersis->Enqueue(pBuff,iPackLen)!=SUCCESS)
        {
            ostrMsgBody=RE_DELETE_EXCHANGE_FAILED+strExchangeName;
            return ERR_LOGIC_SERVER_DELETE_EXCHANGE;
        }
    }
    m_mExchanges.erase(strExchangeName);
    delete pExchange;
    pExchange=NULL;
    ostrMsgBody=RE_DELETE_EXCHANGE_SUCCEED+strExchangeName;
    LogicServerLogger.WriteLog(mq_log_info,"Delete exchange succeed,exchange name is %s",pExchangeName);
    LogicServerLogger.Print(mq_log_info,"Delete exchange succeed,exchange name is %s",pExchangeName);
    return SUCCESS;
}

int LogicServer::OnDeleteQueue(char *ipBuffer,int iLen,string &ostrMsgBody)
{
    char *pTemp=ipBuffer;
    //读取queue名称
    char pQueueName[MAX_NAME_LENGTH];
    memset(pQueueName,0,sizeof(pQueueName));
    FuncTool::ReadBuf(pTemp,pQueueName,sizeof(pQueueName));
    string strQueueName=pQueueName;

    LogicServerLogger.WriteLog(mq_log_info,"begain delete queue,queue name is %s",pQueueName);
    LogicServerLogger.Print(mq_log_info,"begain delete queue,queue name is %s",pQueueName);

     //检查queue是否存在
    MessageQueue* pMsgQueue=NULL;
    if(m_mUnSubcribeQueues.find(strQueueName)!=m_mUnSubcribeQueues.end())
    {
        pMsgQueue=m_mUnSubcribeQueues[strQueueName];
        m_mUnSubcribeQueues.erase(strQueueName);
    }
    if(m_mSubcribeQueues.find(strQueueName)!=m_mSubcribeQueues.end())
    {
        pMsgQueue=m_mSubcribeQueues[strQueueName];
        m_mSubcribeQueues.erase(strQueueName);
    }
    if(pMsgQueue==NULL)
    {
        LogicServerLogger.WriteLog(mq_log_err,"queue isn't exist,name is %s",pQueueName);
        LogicServerLogger.Print(mq_log_err,"queue isn't exist,name is %s",pQueueName);
        ostrMsgBody=RE_QUEUE_NOT_EXIST+strQueueName;
        return ERR_LOGIC_SERVER_DELETE_QUEUE;
    }
    //若是需要持久化的queue，则发送删除消息
    if(pMsgQueue->IsDurable())
    {
        char pBuff[MAX_CLINT_PACKAGE_LENGTH];
        memset(pBuff,0,sizeof(pBuff));
        unsigned short iPackLen=sizeof(unsigned short)+sizeof(unsigned short)+iLen;
        pTemp=pBuff;
        int offset=FuncTool::WriteShort(pTemp,iPackLen);
        pTemp+=offset;
        offset=FuncTool::WriteShort(pTemp,CMD_DELETE_QUEUE);
        pTemp+=offset;
        offset=FuncTool::WriteBuf(pTemp,ipBuffer,iLen);
        if(m_pQueueToPersis->Enqueue(pBuff,iPackLen)!=SUCCESS)
        {
            ostrMsgBody=RE_DELETE_QUEUE_FAILED+strQueueName;
            return ERR_LOGIC_SERVER_DELETE_QUEUE;
        }
    }
    //清空消息删除队列
    pMsgQueue->ClearMessage();
    //遍历所有exchange，删除对应队列的绑定
    unordered_map<string,Exchange *>::iterator itr=m_mExchanges.begin();
    while(itr!=m_mExchanges.end())
    {
        Exchange *pExchange=itr->second;
        if(pExchange->GetBindingSize()==0)
        {
            itr++;
        }
        else
        {
            pExchange->DeleteBindingQueue(pMsgQueue);
            //若删除后交换器为空，且交换器也是自动删除的，则删除exchange
            if(pExchange->GetBindingSize()==0&&pExchange->IsAutoDel())
            {
                m_mExchanges.erase(itr++);
                delete pExchange;
            }
            else
            {
                itr++;
            }
        }
    }
    delete pMsgQueue;
    pMsgQueue=NULL;
    ostrMsgBody=RE_DELETE_QUEUE_SUCCEED+strQueueName;
    LogicServerLogger.WriteLog(mq_log_info,"Delete queue succeed,queue name is %s",pQueueName);
    LogicServerLogger.Print(mq_log_info,"Delete queue succeed,queue name is %s",pQueueName);
    return SUCCESS;
}

int LogicServer::OnCancelSubscribe(char *ipBuffer,int iLen,int iCliIndex,string &ostrMsgBody)
{
    char *pTemp=ipBuffer;
    //读取queue名称
    char pQueueName[MAX_NAME_LENGTH];
    memset(pQueueName,0,sizeof(pQueueName));
    FuncTool::ReadBuf(pTemp,pQueueName,sizeof(pQueueName));
    string strQueueName=pQueueName;

    LogicServerLogger.WriteLog(mq_log_info,"begain cancel subscribe,queue name is %s",pQueueName);
    LogicServerLogger.Print(mq_log_info,"begain cancel subscribe,queue name is %s",pQueueName);

    //看是否已经订阅
    if(m_mUnSubcribeQueues.find(strQueueName)!=m_mUnSubcribeQueues.end())
    {
        LogicServerLogger.WriteLog(mq_log_err,"queue isn't subscribed,name is %s",pQueueName);
        LogicServerLogger.Print(mq_log_err,"queue isn't subscribed,name is %s",pQueueName);
        ostrMsgBody=RE_QUEUE_NOT_SUBSCRIBE+strQueueName;
        return ERR_LOGIC_SERVER_CANCEL_SUBSCRIBE;
    }

    //检查queue是否存在
    MessageQueue* pMsgQueue=NULL;
    if(m_mSubcribeQueues.find(strQueueName)!=m_mSubcribeQueues.end())
    {
        pMsgQueue=m_mSubcribeQueues[strQueueName];
    }
    if(pMsgQueue==NULL)
    {
        LogicServerLogger.WriteLog(mq_log_err,"queue isn't exist,name is %s",pQueueName);
        LogicServerLogger.Print(mq_log_err,"queue isn't exist,name is %s",pQueueName);
        ostrMsgBody=RE_QUEUE_NOT_EXIST+strQueueName;
        return ERR_LOGIC_SERVER_CANCEL_SUBSCRIBE;
    }
    int iConfirmLevel=pMsgQueue->GetConfirmLevel();
    pMsgQueue->CancelSubscribe(iCliIndex);
    //若是需要持久化的queue,则将当前订阅消费者发送给用户
    if(pMsgQueue->IsDurable())
    {
        char pBuff[MAX_CLINT_PACKAGE_LENGTH];
        memset(pBuff,0,sizeof(pBuff));
        int iLen=MAX_CLINT_PACKAGE_LENGTH;
        pMsgQueue->SerializeSubcribeToString(pBuff,iLen,false);
        if(m_pQueueToPersis->Enqueue(pBuff,iLen)!=SUCCESS)
        {
            pMsgQueue->AddSubscribe(iCliIndex,iConfirmLevel);
            ostrMsgBody=RE_CANCEL_SUBSCRIBE_FAILED+strQueueName;
            return ERR_LOGIC_SERVER_CANCEL_SUBSCRIBE;
        }
    }
    ostrMsgBody=RE_CANCEL_SUBSCRIBE_SUCCEED+strQueueName;
    LogicServerLogger.WriteLog(mq_log_info,"cancel subscribe succeed,queue name is %s",pQueueName);
    LogicServerLogger.Print(mq_log_info,"cancel subscribe succeed,queue name is %s",pQueueName);
    //将队列从订阅队列中取出
    m_mSubcribeQueues.erase(pMsgQueue->GetQueueName());
    //若是自动删除队列，则在订阅对象为空时，清空队列消息并删除队列
    if(pMsgQueue->IsAutoDel()&&!pMsgQueue->IsSubscribe())
    {
        pMsgQueue->ClearMessage();
        //遍历所有exchange，删除对应队列的绑定
        unordered_map<string,Exchange *>::iterator itr=m_mExchanges.begin();
        while(itr!=m_mExchanges.end())
        {
            Exchange *pExchange=itr->second;
            if(pExchange->GetBindingSize()==0)
            {
                itr++;
            }
            else
            {
                pExchange->DeleteBindingQueue(pMsgQueue);
                //若删除后交换器为空，且交换器也是自动删除的，则删除exchange
                if(pExchange->GetBindingSize()==0&&pExchange->IsAutoDel())
                {
                    //若是需要持久化的exchange，则发送删除消息
                    if(pExchange->IsDurable())
                    {
                        string strExchangeName=pExchange->GetExchangeName();
                        char pBuff[MAX_CLINT_PACKAGE_LENGTH];
                        memset(pBuff,0,sizeof(pBuff));
                        unsigned short iPackLen=sizeof(unsigned short)+sizeof(unsigned short)+strExchangeName.size();
                        pTemp=pBuff;
                        int offset=FuncTool::WriteShort(pTemp,iPackLen);
                        pTemp+=offset;
                        offset=FuncTool::WriteShort(pTemp,CMD_DELETE_EXCHANGE);
                        pTemp+=offset;
                        offset=FuncTool::WriteBuf(pTemp,strExchangeName.c_str(),strExchangeName.size());
                        if(m_pQueueToPersis->Enqueue(pBuff,iPackLen)!=SUCCESS)
                        {
                            ostrMsgBody=RE_DELETE_EXCHANGE_FAILED+strExchangeName;
                            return ERR_LOGIC_SERVER_DELETE_EXCHANGE;
                        }
                    }
                    m_mExchanges.erase(itr++);
                    delete pExchange;
                }
                else
                {
                    itr++;
                }
            }
        }
        //若是需要持久化的queue，则发送删除消息
        if(pMsgQueue->IsDurable())
        {
            char pBuff[MAX_CLINT_PACKAGE_LENGTH];
            memset(pBuff,0,sizeof(pBuff));
            unsigned short iPackLen=sizeof(unsigned short)+sizeof(unsigned short)+strQueueName.size();
            pTemp=pBuff;
            int offset=FuncTool::WriteShort(pTemp,iPackLen);
            pTemp+=offset;
            offset=FuncTool::WriteShort(pTemp,CMD_DELETE_QUEUE);
            pTemp+=offset;
            offset=FuncTool::WriteBuf(pTemp,strQueueName.c_str(),strQueueName.size());
            if(m_pQueueToPersis->Enqueue(pBuff,iPackLen)!=SUCCESS)
            {
                ostrMsgBody=RE_DELETE_QUEUE_FAILED+strQueueName;
                return ERR_LOGIC_SERVER_DELETE_QUEUE;
            }
        }
        delete pMsgQueue;
    }
    else if(!pMsgQueue->IsSubscribe())
    {
        //没有消费者订阅时放入未订阅队列
        m_mUnSubcribeQueues[pMsgQueue->GetQueueName()]=pMsgQueue;
    }
    return SUCCESS;
}

int LogicServer::OnClientAck(char *ipBuffer,int iLen,int iCliIndex)
{
    char *pTemp=ipBuffer;
    //读确认信息
    unsigned char confirmLevel=0;
    int offset=FuncTool::ReadByte(pTemp,confirmLevel);
    pTemp+=offset;
    int iAckSeq;
    FuncTool::ReadInt(pTemp,iAckSeq);
    //单个确认
    if(confirmLevel==CONSUMER_AUTO_ACK||CONSUMER_SINGLE_ACK)
    {
        //从待确认链表中找到对应消息进行移除
        list<SeverStoreMessage *>::iterator itr=m_NeedAckMessageList.begin();
        while(itr!=m_NeedAckMessageList.end())
        {
            if((*itr)->GetMsgSeq()==iAckSeq)
            {
                LogicServerLogger.WriteLog(mq_log_info,"recved client ack,seq is %d",iAckSeq);
                LogicServerLogger.Print(mq_log_info,"recved client ack,seq is %d",iAckSeq);
                SeverStoreMessage*pMsg=*itr;
                m_NeedAckMessageList.erase(itr++);
                //若消息是持久化的，则向持久化层发包
                if(pMsg->IsDurable())
                {
                    string strQueueName=pMsg->GetBelongQueueName();
                    char pBuff[MAX_CLINT_PACKAGE_LENGTH];
                    memset(pBuff,0,sizeof(pBuff));
                    unsigned short iPackLen=sizeof(unsigned short)+sizeof(unsigned short)+MAX_NAME_LENGTH+sizeof(int);
                    char *pTemp=pBuff;
                    int offset=FuncTool::WriteShort(pTemp,iPackLen);
                    pTemp+=offset;
                    offset=FuncTool::WriteShort(pTemp,CMD_CLIENT_ACK_MESSAGE);
                    pTemp+=offset;
                    offset=FuncTool::WriteBuf(pTemp,strQueueName.c_str(),strQueueName.size());
                    pTemp+=MAX_NAME_LENGTH;
                    offset=FuncTool::WriteInt(pTemp,pMsg->GetDurableIndex());
                    m_pQueueToPersis->Enqueue(pBuff,iPackLen);
                }
                delete pMsg;
                break;
            }
            else
            {
                ++itr;
            }
        }
    }
    //批量确认
    else if(confirmLevel==CONSUMER_MUTIL_ACK)
    {
        //从待确认链表中找到同一客户发送的序号小于它的消息并删除
        list<SeverStoreMessage *>::iterator itr=m_NeedAckMessageList.begin();
        while(itr!=m_NeedAckMessageList.end())
        {
            if((*itr)->GetClientIndex()!=iCliIndex)
            {
                ++itr;
                continue;
            }
            if((*itr)->GetMsgSeq()<=iAckSeq)
            {
                LogicServerLogger.WriteLog(mq_log_info,"recved client ack,seq is %d",iAckSeq);
                LogicServerLogger.Print(mq_log_info,"recved client ack,seq is %d",iAckSeq);
                SeverStoreMessage*pMsg=*itr;
                m_NeedAckMessageList.erase(itr++);
                //若消息是持久化的，则向持久化层发包
                if(pMsg->IsDurable())
                {
                    string strQueueName=pMsg->GetBelongQueueName();
                    char pBuff[MAX_CLINT_PACKAGE_LENGTH];
                    memset(pBuff,0,sizeof(pBuff));
                    unsigned short iPackLen=sizeof(unsigned short)+sizeof(unsigned short)+MAX_NAME_LENGTH+sizeof(int);
                    char *pTemp=pBuff;
                    int offset=FuncTool::WriteShort(pTemp,iPackLen);
                    pTemp+=offset;
                    offset=FuncTool::WriteShort(pTemp,CMD_CLIENT_ACK_MESSAGE);
                    pTemp+=offset;
                    offset=FuncTool::WriteBuf(pTemp,strQueueName.c_str(),strQueueName.size());
                    pTemp+=MAX_NAME_LENGTH;
                    offset=FuncTool::WriteInt(pTemp,pMsg->GetDurableIndex());
                    m_pQueueToPersis->Enqueue(pBuff,iPackLen);
                }
                delete pMsg;
            }
            else
            {
                break;
            }
        }
    }
    
    return SUCCESS;
}

int LogicServer::OnClientExit(int iClientIndex)
{
    //若退出的是生产者，则需要在已收到确认消息集合中删除掉该生产者
    unordered_set<string>::iterator itr1=m_producerMsgSeqs.begin();
    string strClientIndex=std::to_string(iClientIndex);
    while(itr1!=m_producerMsgSeqs.end())
    {
        string strMsgSeq=*itr1;
        if(strncmp(strClientIndex.c_str(),strMsgSeq.c_str(),strClientIndex.size())==0)
        {
            m_producerMsgSeqs.erase(itr1++);
        }
        else
        {
            itr1++;
        }
    }

    //若退出的是消费者，遍历所有队列，取消该消费者的订阅
    unordered_map<string,MessageQueue *>::iterator itr2=m_mSubcribeQueues.begin();
    while(itr2!=m_mSubcribeQueues.end())
    {
        MessageQueue *pMsgQueue=itr2->second;
        int iOriNum=pMsgQueue->GetSubscribeSize();
        pMsgQueue->CancelSubscribe(iClientIndex);
        int iNewNum=pMsgQueue->GetSubscribeSize();
        if(iOriNum>iNewNum)
        {
            //说明该客户订阅了队列
            //若是需要持久化的queue,则将当前订阅消费者发送给持久化层
            if(pMsgQueue->IsDurable())
            {
                char pBuff[MAX_CLINT_PACKAGE_LENGTH];
                memset(pBuff,0,sizeof(pBuff));
                int iLen=MAX_CLINT_PACKAGE_LENGTH;
                pMsgQueue->SerializeSubcribeToString(pBuff,iLen,false);
                m_pQueueToPersis->Enqueue(pBuff,iLen);
            }
            //若正好是最后一个用户，现将队列从订阅集合中移除
            if(iNewNum==0)
            {
                m_mSubcribeQueues.erase(itr2++);
                //若队列是自动删除的
                if(pMsgQueue->IsAutoDel())
                {
                    pMsgQueue->ClearMessage();
                    //遍历所有exchange，删除对应队列的绑定
                    unordered_map<string,Exchange *>::iterator itr=m_mExchanges.begin();
                    while(itr!=m_mExchanges.end())
                    {
                        Exchange *pExchange=itr->second;
                        if(pExchange->GetBindingSize()==0)
                        {
                            itr++;
                        }
                        else
                        {
                            pExchange->DeleteBindingQueue(pMsgQueue);
                            //若删除后交换器为空，且交换器也是自动删除的，则删除exchange
                            if(pExchange->GetBindingSize()==0&&pExchange->IsAutoDel())
                            {
                                //若是需要持久化的exchange，则发送删除消息
                                if(pExchange->IsDurable())
                                {
                                    string strExchangeName=pExchange->GetExchangeName();
                                    char pBuff[MAX_CLINT_PACKAGE_LENGTH];
                                    memset(pBuff,0,sizeof(pBuff));
                                    unsigned short iPackLen=sizeof(unsigned short)+sizeof(unsigned short)+strExchangeName.size();
                                    char *pTemp=pBuff;
                                    int offset=FuncTool::WriteShort(pTemp,iPackLen);
                                    pTemp+=offset;
                                    offset=FuncTool::WriteShort(pTemp,CMD_DELETE_EXCHANGE);
                                    pTemp+=offset;
                                    offset=FuncTool::WriteBuf(pTemp,strExchangeName.c_str(),strExchangeName.size());
                                    m_pQueueToPersis->Enqueue(pBuff,iPackLen);
                                }
                                m_mExchanges.erase(itr++);
                                delete pExchange;
                            }
                            else
                            {
                                itr++;
                            }
                        }
                    }
                    //若是需要持久化的queue，则发送删除消息
                    if(pMsgQueue->IsDurable())
                    {
                        string strQueueName =pMsgQueue->GetQueueName();
                        char pBuff[MAX_CLINT_PACKAGE_LENGTH];
                        memset(pBuff,0,sizeof(pBuff));
                        unsigned short iPackLen=sizeof(unsigned short)+sizeof(unsigned short)+strQueueName.size();
                        char *pTemp=pBuff;
                        int offset=FuncTool::WriteShort(pTemp,iPackLen);
                        pTemp+=offset;
                        offset=FuncTool::WriteShort(pTemp,CMD_DELETE_QUEUE);
                        pTemp+=offset;
                        offset=FuncTool::WriteBuf(pTemp,strQueueName.c_str(),strQueueName.size());
                        m_pQueueToPersis->Enqueue(pBuff,iPackLen);
                    }
                    delete pMsgQueue;
                }
                else
                {
                    //若不是自动删除的，则放入未订阅队列集合
                    m_mUnSubcribeQueues[pMsgQueue->GetQueueName()]=pMsgQueue;
                }           
            }
            else
            {
                ++itr2;
            }
        }
        else
        {
            ++itr2;
        }
    }
    //若退出是消费者，则还需要遍历待确认链表，将消息放回原先队列
    list<SeverStoreMessage *>::iterator itr3=m_NeedAckMessageList.begin();
    while(itr3!=m_NeedAckMessageList.end())
    {
        SeverStoreMessage *pMsg=*itr3;
        if(pMsg->GetClientIndex()==iClientIndex)
        {
            string strQueueName=pMsg->GetBelongQueueName();
            MessageQueue *pMsgQueue=NULL;
            if(m_mUnSubcribeQueues.find(strQueueName)!=m_mUnSubcribeQueues.end())
            {
                pMsgQueue=m_mUnSubcribeQueues[strQueueName];
            }
            if(m_mSubcribeQueues.find(strQueueName)!=m_mSubcribeQueues.end())
            {
                pMsgQueue=m_mSubcribeQueues[strQueueName];
            }
            if(pMsgQueue)
            {
                pMsgQueue->AddMessageFront(pMsg);
            }
            m_NeedAckMessageList.erase(itr3++);
        }
        else
        {
            itr3++;
        }
    }
    return SUCCESS;
}

int LogicServer::ProcessRecvData(char *ipBuffer,int iLen,int &opClientIndex,string &ostrMsgBody)
{
    ClientPackageHead header;
    //读取消息长度
    char *pTemp=ipBuffer;
    int offset=FuncTool::ReadShort(pTemp,header.m_iPackLen);
    //读取长度不足，直接退出
    if(header.m_iPackLen>iLen)
    {
        return ERR_LOGIC_SERVER_PROCESS;
    }
    //查找对应客户端下标
    pTemp+=offset;
    offset=FuncTool::ReadInt(pTemp,header.m_iClientIndex);
    opClientIndex=header.m_iClientIndex;

    //读取消息类型
    pTemp+=offset;
    offset=FuncTool::ReadShort(pTemp,header.m_iCmdId);
    pTemp+=offset;

    LogicServerLogger.WriteLog(mq_log_info,"msg len is %d,client index is %d,msg type is %d",
    header.m_iPackLen,header.m_iClientIndex,header.m_iCmdId);
    LogicServerLogger.Print(mq_log_info,"msg len is %d,client index is %d,msg type is %d",
    header.m_iPackLen,header.m_iClientIndex,header.m_iCmdId);

    int ret=SUCCESS;
    switch(header.m_iCmdId)
    {
        case CMD_CREATE_EXCNANGE:
        {
            ret=OnCreateExchange(pTemp,iLen-sizeof(header),ostrMsgBody);
            break;
        }
        case CMD_CREATE_QUEUE:
        {
            ret=OnCreateQueue(pTemp,iLen-sizeof(header),ostrMsgBody);
            break;
        }
        case CMD_CREATE_BINDING: 
        {
            ret=OnCreateBinding(pTemp,iLen-sizeof(header),ostrMsgBody);
            break;
        }
        case CMD_CREATE_PUBLISH: 
        {
            ret=OnPublishMessage(pTemp,iLen-sizeof(header),header.m_iClientIndex,ostrMsgBody);
            break;
        }
        case CMD_CREATE_RECV: 
        {
            ret=OnRecvMessage(pTemp,iLen-sizeof(header),header.m_iClientIndex,ostrMsgBody);
            break;
        }
        case CMD_CREATE_SUBCRIBE: 
        {
            ret=OnCreateSubscribe(pTemp,iLen-sizeof(header),header.m_iClientIndex,ostrMsgBody);
            break;
        }
        case CMD_DELETE_EXCHANGE:
        {
            ret=OnDeleteExchange(pTemp,iLen-sizeof(header),ostrMsgBody);
            break;
        }
        case CMD_DELETE_QUEUE:
        {
            ret=OnDeleteQueue(pTemp,iLen-sizeof(header),ostrMsgBody);
            break;
        }
        case CMD_CANCEL_SUBCRIBE:
        {
            ret=OnCancelSubscribe(pTemp,iLen-sizeof(header),header.m_iClientIndex,ostrMsgBody);
            break;
        }
        case CMD_CLIENT_ACK_MESSAGE:
        {
            ret=OnClientAck(pTemp,iLen-sizeof(header),header.m_iClientIndex);
            break;
        }
        case CMD_CLIENT_EXIT:
        {
            ret=OnClientExit(header.m_iClientIndex);
            break;
        }
        default:
            return ERR_LOGIC_SERVER_PROCESS;
    }
    return ret;
}

int LogicServer::ReSendMsg()
{
    struct timeval tNow;
    gettimeofday(&tNow,NULL);
    //从待确认链表中找到对应消息进行移除
    list<SeverStoreMessage *>::iterator itr=m_NeedAckMessageList.begin();
    while(itr!=m_NeedAckMessageList.end())
    {
        SeverStoreMessage *pMsg=*itr;
        //获取等待确认花费时间
        struct timeval tSaveTime=pMsg->GetSaveTime();
        int iWaitTime=((tNow.tv_sec-tSaveTime.tv_sec)*1000000+(tNow.tv_usec-tSaveTime.tv_usec))/1000;
        //3秒未确认则重发
        if(iWaitTime>=3*1000*1000)
        {
            char pBuffer[MAX_CLINT_PACKAGE_LENGTH];
            memset(pBuffer,0,sizeof(pBuffer));
            int iLen=MAX_CLINT_PACKAGE_LENGTH;
            pMsg->GetMessagePack(pBuffer,&iLen);
            pMsg->SetSaveTime();
            m_NeedAckMessageList.push_back(pMsg);
            m_NeedAckMessageList.erase(itr++);
            //消息放入队列
            m_pQueueToConnect->Enqueue(pBuffer,iLen);
            LogicServerLogger.WriteLog(mq_log_info,"resend message to client,msg seq is %d",pMsg->GetMsgSeq());
            LogicServerLogger.Print(mq_log_info,"resend message to client,msg seq is %d",pMsg->GetMsgSeq());
        }
        else
        {
            break;
        }
    }
    return SUCCESS;
}

// int LogicServer::OnStop()
// {
//     //记录所有创建的exchange
//     LogicServerLogger.WriteLog(mq_log_info,"begain stop save");
//     LogicServerLogger.Print(mq_log_info,"begain stop save");
//     LogicServerLogger.WriteLog(mq_log_info,"begain save exchange");
//     LogicServerLogger.Print(mq_log_info,"begain save exchange");

//     //写方式打开创建文件
//     fstream outputEx;
//     string strExchangeFile=PATH_STOP_RECORD+FILE_STOP_SAVE_EXCHANGE;
//     outputEx.open(strExchangeFile.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
//     unordered_map<string,Exchange *>::iterator itr1=m_mExchanges.begin();
//     string strOut;
//     char pBuff[MAX_RDWR_FILE_BUFF_SIZE];
//     //memset(pBuff,0,MAX_RDWR_FILE_BUFF_SIZE);
//     FuncTool::WriteInt(pBuff,10);
//     outputEx.write("wyl",4);
//     outputEx.write(pBuff,10);
//     while(itr1!=m_mExchanges.end())
//     {
//         Exchange *pExchange=itr1->second;
//         if(pExchange->SerializeToString(strOut)==SUCCESS)
//         {
//             outputEx<<strOut;
//             LogicServerLogger.WriteLog(mq_log_info," exchange str is %s",strOut.c_str());
//             LogicServerLogger.Print(mq_log_info,"exchange str is %s",strOut.c_str());
//         }
//         ++itr1;
//     }
//     outputEx.sync();
//     outputEx.close();

//     LogicServerLogger.WriteLog(mq_log_info,"begain save queue and msg");
//     LogicServerLogger.Print(mq_log_info,"begain save queue and msg");
//     //记录所有创建的queue及message
//     fstream outputQueue;
//     fstream outputMsg;
//     string strQueueFile=PATH_STOP_RECORD+FILE_STOP_SAVE_QUEUE;
//     string strMsgFile=PATH_STOP_RECORD+FILE_STOP_SAVE_MESSAGE;
//     outputQueue.open(strQueueFile.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
//     outputMsg.open(strMsgFile.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
//     unordered_map<string,MessageQueue *>::iterator itr2=m_mSubcribeQueues.begin();
//     while(itr2!=m_mSubcribeQueues.end())
//     {
//         MessageQueue *pMsgQueue=itr2->second;
//         if(pMsgQueue->SerializeToString(strOut)==SUCCESS)
//         {
//             outputQueue<<strOut;
//             LogicServerLogger.WriteLog(mq_log_info," queue str is %s",strOut.c_str());
//             LogicServerLogger.Print(mq_log_info,"queue str is %s",strOut.c_str());
//         }
//         while(!pMsgQueue->IsEmpty())
//         {
//             SeverStoreMessage *pMsg=pMsgQueue->PopFrontMsg();
//            if(pMsg->SerializeToString(pMsgQueue->GetQueueName(),strOut)==SUCCESS)
//            {
//                 outputMsg<<strOut;
//                 LogicServerLogger.WriteLog(mq_log_info," msg str is %s",strOut.c_str());
//                 LogicServerLogger.Print(mq_log_info,"msg str is %s",strOut.c_str());
//            }
//         }
//         ++itr2;
//     }
//     itr2=m_mUnSubcribeQueues.begin();
//     while(itr2!=m_mUnSubcribeQueues.end())
//     {
//         MessageQueue *pMsgQueue=itr2->second;
//         if(pMsgQueue->SerializeToString(strOut)==SUCCESS)
//         {
//             outputQueue<<strOut;
//             LogicServerLogger.WriteLog(mq_log_info," queue str is %s",strOut.c_str());
//             LogicServerLogger.Print(mq_log_info,"queue str is %s",strOut.c_str());
//         }
//         while(!pMsgQueue->IsEmpty())
//         {
//             SeverStoreMessage *pMsg=pMsgQueue->PopFrontMsg();
//            if(pMsg->SerializeToString(pMsgQueue->GetQueueName(),strOut)==SUCCESS)
//            {
//                 outputMsg<<strOut;
//                 LogicServerLogger.WriteLog(mq_log_info," msg str is %s",strOut.c_str());
//                 LogicServerLogger.Print(mq_log_info,"msg str is %s",strOut.c_str());
//            }
//         }
//         ++itr2;
//     }
//     outputQueue.sync();
//     outputQueue.close();
//     outputMsg.sync();
//     outputMsg.close();
//     LogicServerLogger.WriteLog(mq_log_info,"end stop save");
//     LogicServerLogger.Print(mq_log_info,"end stop save");
//     return SUCCESS;
// }

int LogicServer::OnStop()
{
    // //记录所有创建的exchange
    // LogicServerLogger.WriteLog(mq_log_info,"begain stop save");
    // LogicServerLogger.Print(mq_log_info,"begain stop save");
    // LogicServerLogger.WriteLog(mq_log_info,"begain save exchange");
    // LogicServerLogger.Print(mq_log_info,"begain save exchange");

    // //写方式打开创建文件
    // string strExchangeFile=PATH_STOP_RECORD+FILE_STOP_SAVE_EXCHANGE;
    // FILE *pExFile=fopen(strExchangeFile.c_str(),"w+");
    // if(pExFile==NULL)
    // {
    //     return ERROR;
    // }
    // unordered_map<string,Exchange *>::iterator itr1=m_mExchanges.begin();
    // char pBuff[MAX_RDWR_FILE_BUFF_SIZE];
    // int iRet=0;
    // while(itr1!=m_mExchanges.end())
    // {
    //     int iLen=MAX_RDWR_FILE_BUFF_SIZE;
    //     Exchange *pExchange=itr1->second;
    //     memset(pBuff,0,MAX_RDWR_FILE_BUFF_SIZE);
    //     if(pExchange->SerializeToString(pBuff,iLen)==SUCCESS)
    //     {
    //         iRet=fwrite(pBuff,iLen,1,pExFile);
    //         LogicServerLogger.WriteLog(mq_log_info,"save exchange len is %d:%d",iLen,iRet);
    //         LogicServerLogger.Print(mq_log_info,"save exchange len is %d:%d",iLen,iRet);
    //     }
    //     ++itr1;
    // }
    // fflush(pExFile);
    // fclose(pExFile);

    // LogicServerLogger.WriteLog(mq_log_info,"begain save queue and msg");
    // LogicServerLogger.Print(mq_log_info,"begain save queue and msg");
    // //记录所有创建的queue及message
    // string strQueueFile=PATH_STOP_RECORD+FILE_STOP_SAVE_QUEUE;
    // string strMsgFile=PATH_STOP_RECORD+FILE_STOP_SAVE_MESSAGE;
    // FILE *pQueueFile=fopen(strQueueFile.c_str(),"w+");
    // FILE *pMsgFile=fopen(strMsgFile.c_str(),"w+");
    // unordered_map<string,MessageQueue *>::iterator itr2=m_mSubcribeQueues.begin();
    // while(itr2!=m_mSubcribeQueues.end())
    // {
    //     memset(pBuff,0,MAX_RDWR_FILE_BUFF_SIZE);
    //     int iLen=MAX_RDWR_FILE_BUFF_SIZE;
    //     MessageQueue *pMsgQueue=itr2->second;
    //     if(pMsgQueue->SerializeToString(pBuff,iLen)==SUCCESS)
    //     {
    //         iRet=fwrite(pBuff,iLen,1,pQueueFile);
    //         LogicServerLogger.WriteLog(mq_log_info,"save queue len is %d:%d",iLen,iRet);
    //         LogicServerLogger.Print(mq_log_info,"save queue len is %d:%d",iLen,iRet);
    //     }
    //     while(!pMsgQueue->IsEmpty())
    //     {
    //         memset(pBuff,0,MAX_RDWR_FILE_BUFF_SIZE);
    //         int iLen=MAX_RDWR_FILE_BUFF_SIZE;
    //         SeverStoreMessage *pMsg=pMsgQueue->PopFrontMsg();
    //         if(pMsg->SerializeToString(pMsgQueue->GetQueueName(),pBuff,iLen)==SUCCESS)
    //         {
    //             iRet=fwrite(pBuff,iLen,1,pMsgFile);
    //             LogicServerLogger.WriteLog(mq_log_info,"save msg len is %d:%d",iLen,iRet);
    //             LogicServerLogger.Print(mq_log_info,"save msg len is %d:%d",iLen,iRet);
    //         }
    //     }
    //     ++itr2;
    // }

    // itr2=m_mUnSubcribeQueues.begin();
    // while(itr2!=m_mUnSubcribeQueues.end())
    // {
    //     memset(pBuff,0,MAX_RDWR_FILE_BUFF_SIZE);
    //     int iLen=MAX_RDWR_FILE_BUFF_SIZE;
    //     MessageQueue *pMsgQueue=itr2->second;
    //     if(pMsgQueue->SerializeToString(pBuff,iLen)==SUCCESS)
    //     {
    //         iRet=fwrite(pBuff,iLen,1,pQueueFile);
    //         LogicServerLogger.WriteLog(mq_log_info,"save queue len is %d:%d",iLen,iRet);
    //         LogicServerLogger.Print(mq_log_info,"save queue len is %d:%d",iLen,iRet);
    //     }
    //     while(!pMsgQueue->IsEmpty())
    //     {
    //         memset(pBuff,0,MAX_RDWR_FILE_BUFF_SIZE);
    //         int iLen=MAX_RDWR_FILE_BUFF_SIZE;
    //         SeverStoreMessage *pMsg=pMsgQueue->PopFrontMsg();
    //         if(pMsg->SerializeToString(pMsgQueue->GetQueueName(),pBuff,iLen)==SUCCESS)
    //         {
    //                 iRet=fwrite(pBuff,iLen,1,pMsgFile);
    //                 LogicServerLogger.WriteLog(mq_log_info,"save msg len is %d:%d",iLen,iRet);
    //                 LogicServerLogger.Print(mq_log_info,"save msg len is %d:%d",iLen,iRet);
    //         }
    //     }
    //     ++itr2;
    // }
    // fflush(pQueueFile);
    // fclose(pQueueFile);
    // fflush(pMsgFile);
    // fclose(pMsgFile);

    // LogicServerLogger.WriteLog(mq_log_info,"end stop save");
    // LogicServerLogger.Print(mq_log_info,"end stop save");
    // return SUCCESS;
}

int LogicServer::Run()
{
    while (!m_bStop)
    {
        LogicServerLogger.WriteLog(mq_log_info,"begain loop...");
        LogicServerLogger.Print(mq_log_info,"begain loop...");
        //先处理消息队列中的订阅消息，然后再处理共享内存队列中的消息
        bool bPushEmpty=true;
        bool bShmQueue=true;
        LogicSrvTimer controlTimer;
        LogicServerLogger.WriteLog(mq_log_info,"begain push msg...");
        LogicServerLogger.Print(mq_log_info,"begain push msg...");
        controlTimer.Begain();
        if(!m_mSubcribeQueues.empty())
        {
            PushMessage();
        }
        controlTimer.PushMessageDown();
        LogicServerLogger.WriteLog(mq_log_info,"begain process shmqueue data...");
        LogicServerLogger.Print(mq_log_info,"begain process shmqueue data...");
        controlTimer.GetMaxTimeForQueueData();
        bool bRecvData=true;
        int iDateRecvCount=0;
        char pBuff[MAX_CLINT_PACKAGE_LENGTH];
        memset(pBuff,0,MAX_CLINT_PACKAGE_LENGTH);
        int i=1;
        //最多接收800个包
        while(iDateRecvCount<800)
        {
            int iLen=MAX_CLINT_PACKAGE_LENGTH;
            int ret=m_pQueueFromConnect->Dequeue(pBuff,&iLen);
            if(ret==ShmQueue::ERR_SHM_QUEUE_EMPTY)
            {
                LogicServerLogger.WriteLog(mq_log_info,"shmqueue from connect server is empty...");
                LogicServerLogger.Print(mq_log_info,"shmqueue from connect server is empty...");
                bShmQueue=(iDateRecvCount==0);
                break;
            }
            else if(ret!=ShmQueue::SUCCESS)
            {
                LogicServerLogger.WriteLog(mq_log_err,"get data from shmqueue failed,errMsg is %s",m_pQueueFromConnect->GetErrMsg());
                LogicServerLogger.Print(mq_log_err,"get data from shmqueue failed,errMsg is %s",m_pQueueFromConnect->GetErrMsg());
                continue;
            }
            ++m_iRecvCount;
            ++iDateRecvCount;
            //LOG_INFO(0, 0, "get one msg from connect: %s",pBuff);
            //接收到的包长度至少为客户端包头长度
            if(iLen<(int)sizeof(ClientPackageHead))
            {
                LogicServerLogger.WriteLog(mq_log_err,"shmqueue data is tool small");
                LogicServerLogger.Print(mq_log_err,"shmqueue data is tool small");
                continue;
            }
            //处理数据
            string strMsgBody="";
            LogicServerLogger.WriteLog(mq_log_info,"begain process recved data...");
            LogicServerLogger.Print(mq_log_info,"begain process recved data...");
            int iClientIndex=-1;
            ++i;
            //LOG_INFO(0, 0, "begain process msg %d",i);
            ret=ProcessRecvData(pBuff,iLen,iClientIndex,strMsgBody);
            //LOG_INFO(0, 0, "end process msg %d",i);

            //如需要回复，则回复消息
            bool bSucceed=(ret==LogicServer::SUCCESS);
            if(!strMsgBody.empty())
            {
                LogicServerLogger.WriteLog(mq_log_info,"put reply pack into shmqueue,reply msg is %s",strMsgBody.c_str());
                LogicServerLogger.Print(mq_log_info,"put reply pack into shmqueue,reply msg is %s",strMsgBody.c_str());
                ActReplyMessage oMsg(bSucceed,strMsgBody);
                oMsg.SetClientIndex(iClientIndex);
                int iLen=strMsgBody.size()+sizeof(ClientPackageHead)+sizeof(bool)+1;
                char *pBuffer=new char[iLen];
                memset(pBuffer,0,iLen);
                oMsg.GetMessagePack(pBuffer,&iLen);
                pBuffer[iLen-1]='\0';
                if(m_pQueueToConnect->Enqueue(pBuffer,iLen)!=ShmQueue::SUCCESS)
                {
                    LogicServerLogger.WriteLog(mq_log_err,"put reply pack into shmqueue failed,errMsg is %s",m_pQueueToConnect->GetErrMsg());
                    LogicServerLogger.Print(mq_log_err,"put reply pack into shmqueue failed,errMsg is %s",m_pQueueToConnect->GetErrMsg());
                }
                delete[]pBuffer;
            }

            // //每读取100个就检查一下是否还可以继续读
            // if(iDateRecvCount>=100)
            // {
            //     bRecvData=controlTimer.HaveTimeForQueueData();
            // }
        }
        LogicServerLogger.WriteLog(mq_log_info,"process shmqueue data down");
        LogicServerLogger.Print(mq_log_info,"process shmqueue data down");
        controlTimer.QueueDataDown();
        //发送批量确认包
        SendMultiAck();
        if(bPushEmpty&&bShmQueue)
        {
            usleep(1000);
        }
        //
    }
    //循环中跳出，说明要退出了，执行数据保存函数
    //OnStop();
    return SUCCESS;
}

int main(int argc,char *argv[])
{
    //初始化log对象
    char pLogPath[MQ_MAX_PATH_LEN]=DEFAULT_LOGIC_SERVER_LOG_PATH;
    if(LogicServerLogger.Init(pLogPath)!=Logger::SUCCESS)
    {
        printf("logger init failed!");
        return -1;
    }
    int iRet = 0;
    //后台运行进程
    LogicServerLogger.WriteLog(mq_log_info,"logic server init...");
    LogicServerLogger.Print(mq_log_info,"logic server init...");
    int ret=0;
    if((ret=FuncTool::DaemonInit())!=FuncTool::SUCCESS)
    {
        LogicServerLogger.WriteLog(mq_log_err,"Daemoninit faild!,return value is %d",ret);
        LogicServerLogger.Print(mq_log_err,"Daemoninit faild!,return value is %d",ret);
        return -1;
    }
    //开启服务
    LogicServerLogger.WriteLog(mq_log_info,"connect server run...");
    LogicServerLogger.Print(mq_log_info,"connect server run...");
    if((ret=LogicServer::GetInstance()->Init())!=LogicServer::SUCCESS)
    {
        fprintf(stderr,"Logic server init failed!");
        return -1;
    }
    LogicServer::GetInstance()->Run();
    return 0;
}