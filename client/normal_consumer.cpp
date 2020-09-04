

//c/c++
#include<string.h>
#include<stdio.h>

//linux
#include<poll.h>
//user define
#include"../logger/logger.h"
#include"client.h"

using namespace WSMQ;

//全局变量 
Logger consumerLogger;

int main(int argc,char *argv[])
{
    if(InitConf(CONF_FILE_PATH)!=0)
    {
        printf("init conf failed\n");
        return -1;
    }
    char pLogPath[MQ_MAX_PATH_LEN]=DEFAULT_CLI_LOG_PATH;
    if(consumerLogger.Init(pLogPath)!=Logger::SUCCESS)
    {
        printf("logger init failed!");
        return -1;
    }
    int iRet = 0;
    
    char pBuff[MAX_CLINT_PACKAGE_LENGTH];
    int iLen=MAX_CLINT_PACKAGE_LENGTH;
    Consumer *pConsumer=new Consumer();
    if(pConsumer->BuildConnection()!=Consumer::SUCCESS)
    {
        consumerLogger.WriteLog(mq_log_err,"Build Connection failed!errMsg is %s",pConsumer->GetErrMsg());
    }
    //创建队列
    if(pConsumer->CreateQueue("queue1")!=Consumer::SUCCESS)
    {
        consumerLogger.WriteLog(mq_log_err,"Create queue failed!errMsg is %s",pConsumer->GetErrMsg());
    }
    if(pConsumer->CreateQueue("queue2")!=Consumer::SUCCESS)
    {
        consumerLogger.WriteLog(mq_log_err,"Create queue failed!errMsg is %s",pConsumer->GetErrMsg());
    }
    //订阅接收,自动确认
    if(pConsumer->CreateSubscribe("queue1",CONSUMER_NO_ACK)!=Consumer::SUCCESS)
    {
        consumerLogger.WriteLog(mq_log_err,"Create subscribe failed!errMsg is %s",pConsumer->GetErrMsg());
    }
    if(pConsumer->CreateSubscribe("queue2",CONSUMER_NO_ACK)!=Consumer::SUCCESS)
    {
        consumerLogger.WriteLog(mq_log_err,"Create subscribe failed!errMsg is %s",pConsumer->GetErrMsg());
    }
    int ret=0;
    while(true)
    {
        memset(pBuff,0,MAX_CLINT_PACKAGE_LENGTH);
        ret=pConsumer->ConsumeMessage(pBuff,&iLen);
        if(ret!=Consumer::SUCCESS)
        {
            consumerLogger.WriteLog(mq_log_err,"Comsume message failed!errMsg is %s",pConsumer->GetErrMsg());
            break;
        }
        consumerLogger.WriteLog(mq_log_info,"recv msg :%s\n",pBuff);
    }
    if(pConsumer->CancelSubscribe("queue1")!=Client::SUCCESS)
    {
        consumerLogger.WriteLog(mq_log_err,"CancelSubscribe failed!errMsg is %s",pConsumer->GetErrMsg());
    }
    if(pConsumer->CancelSubscribe("queue2")!=Client::SUCCESS)
    {
        consumerLogger.WriteLog(mq_log_err,"CancelSubscribe failed!errMsg is %s",pConsumer->GetErrMsg());
    }
    if(pConsumer->DeleteQueue("queue1")!=Client::SUCCESS)
    {
        consumerLogger.WriteLog(mq_log_err,"DeleteQueue failed!errMsg is %s",pConsumer->GetErrMsg());
    }
    if(pConsumer->DeleteQueue("queue2")!=Client::SUCCESS)
    {
        consumerLogger.WriteLog(mq_log_err,"DeleteQueue failed!errMsg is %s",pConsumer->GetErrMsg());
    }
    delete pConsumer;
    pConsumer=NULL;
}