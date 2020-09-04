
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

//计算消息的延时，以毫秒为单位
double GetDelayTime(string iStrMsg)
{
    //获取当前时间
    struct timeval tNow;
    gettimeofday(&tNow,NULL);
    //解析发送时间
    int i=0;
    int iSec=0;
    int iMic=0;
    while(i<iStrMsg.size())
    {
        if(iStrMsg[i]!='#')
        {
            iSec=iSec*10+(iStrMsg[i]-'0');
        }
        else
        {
            break;
        }
        ++i;
    }
    if(i==iStrMsg.size())
    {
        return -1;
    }
    ++i;
    while(i<iStrMsg.size())
    {
        if(iStrMsg[i]!='#')
        {
            iMic=iMic*10+(iStrMsg[i]-'0');
        }
        else
        {
            break;
        }
        ++i;
    }
    if(i==iStrMsg.size())
    {
        return -1;
    }
    int iDelMic=(tNow.tv_sec-iSec)*1000000+(tNow.tv_usec-iMic);
    return (iDelMic*1.0)/1000;
}

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
    //订阅接收,手动确认
    if(pConsumer->CreateSubscribe("queue1",CONSUMER_AUTO_ACK)!=Consumer::SUCCESS)
    {
        consumerLogger.WriteLog(mq_log_err,"Create subscribe failed!errMsg is %s",pConsumer->GetErrMsg());
    }
    if(pConsumer->CreateSubscribe("queue2",CONSUMER_AUTO_ACK)!=Consumer::SUCCESS)
    {
        consumerLogger.WriteLog(mq_log_err,"Create subscribe failed!errMsg is %s",pConsumer->GetErrMsg());
    }
    int ret=0;
    int i=1;
    double iDelayTime=0;
    while(true)
    {
        memset(pBuff,0,MAX_CLINT_PACKAGE_LENGTH);
        ret=pConsumer->ConsumeMessage(pBuff,&iLen);
        if(ret!=Consumer::SUCCESS)
        {
            consumerLogger.WriteLog(mq_log_err,"Comsume message failed!errMsg is %s",pConsumer->GetErrMsg());
            break;
        }
        iDelayTime+=GetDelayTime(pBuff);
        if(i%5000==0)
        {
            consumerLogger.WriteLog(mq_log_info,"average delay time is %.3f ms\n",iDelayTime/5000);
            iDelayTime=0;
            consumerLogger.WriteLog(mq_log_info," recv the %d msg:%s\n",i,pBuff);
        }
        ++i;
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
