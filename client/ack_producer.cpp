
//c/c++
#include<string.h>
#include<stdio.h>
#include<string>
//linux
#include<signal.h>
#include<unistd.h>
//user define
#include"../logger/logger.h"
#include"client.h"
using namespace WSMQ;
using std::string;
using std::to_string;

//全局变量 
Logger producerLogger;

//获取字符串形式的当前时间
string GetCurTime()
{
    struct timeval sNow;
    gettimeofday(&sNow,NULL);
    string strRes=to_string(sNow.tv_sec);
    strRes+="#";
    strRes+=to_string(sNow.tv_usec);
    strRes+="#";
    return strRes;
}

int main(int argc,char *argv[])
{
    if(InitConf(CONF_FILE_PATH)!=0)
    {
        printf("init conf failed\n");
        return -1;
    }
    int iRet = 0;
    char pLogPath[MQ_MAX_PATH_LEN]=DEFAULT_CLI_LOG_PATH;
    if(producerLogger.Init(pLogPath)!=Logger::SUCCESS)
    {
        printf("logger init failed!");
        return -1;
    }
    Producer *pProducter=new Producer();
    if(pProducter->BuildConnection()!=Producer::SUCCESS)
    {
        producerLogger.WriteLog(mq_log_err,"Build Connection failed!errMsg is %s",pProducter->GetErrMsg());
    }
    if(pProducter->CreateExchange("exchange1",EXCHANGE_TYPE_DIRECT)!=Producer::SUCCESS)
    {
        producerLogger.WriteLog(mq_log_err,"Create exchange failed!errMsg is %s",pProducter->GetErrMsg());
    }
    if(pProducter->CreateQueue("queue2")!=Producer::SUCCESS)
    {
        producerLogger.WriteLog(mq_log_err,"Create queue failed!errMsg is %s",pProducter->GetErrMsg());
    }
    if(pProducter->CreateBinding("exchange1","queue2","mq.test2")!=Producer::SUCCESS)
    {
        producerLogger.WriteLog(mq_log_err,"Create binding failed!errMsg is %s",pProducter->GetErrMsg());
    }
    int i=1;
    char pVal[100];
    string strBase(100,'a');
    //开启确认
    pProducter->SetConfirmLevel(PRODUCER_SINGLE_ACK);
    while(i<=100000)
    {
        memset(pVal,0,sizeof(pVal));
        sprintf(pVal," %d",i);
        string strVal=pVal;
        string strCurTime=GetCurTime();
        string strMsg=strCurTime+strBase+strVal;
        int ret=pProducter->PuslishMessage("exchange1","mq.test2",strMsg);
        if(ret!=Producer::SUCCESS)
        {
            break;
        }
        //每5000条打印一次消息
        if(i%5000==0)
        {
            producerLogger.WriteLog(mq_log_info,"send the %d msg:%s\n",i,strMsg.c_str());
            usleep(1000*10);
        }
        ++i;
        //每发100条获取确认号并处理到期消息
        if(pProducter->GetConfirmLevel()!=PRODUCER_NO_ACK&&i%100==0)
        {
            if(pProducter->RecvServerAck()!=Client::SUCCESS)
            {
                producerLogger.WriteLog(mq_log_err,"recv ack failed!errMsg is %s",pProducter->GetErrMsg());
                return -1;
            }
            if(pProducter->ReSendMsg()!=Client::SUCCESS)
            {
               producerLogger.WriteLog(mq_log_err,"resend msg failed!errMsg is %s",pProducter->GetErrMsg());
                return -1;
            }
        }
    }
    // if(pProducter->DeleteQueue("queue2")!=Client::SUCCESS)
    // {
    //     LOG_INFO(0, 0,"Delete queue failed!errMsg is %s",pProducter->GetErrMsg());
    //     producerLogger.Print(mq_log_err,"Delete queue failed!errMsg is %s",pProducter->GetErrMsg());
    // }
    pProducter->DeleteExchange("exchange1");
    delete pProducter;
    pProducter=NULL;
}