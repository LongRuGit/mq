//c/c++
#include<string.h>
#include<stdio.h>
#include<string>
#include<vector>
#include<unordered_map>
//linux
#include<poll.h>
#include<sys/epoll.h>
#include<sys/signal.h>
#include<unistd.h>
//user define
#include"client.h"
#include"../logger/logger.h"
using namespace WSMQ;
using std::string;
using std::vector;
using std::unordered_map;

//全局变量 
Logger consumerLogger;
bool bStop=false;
bool bAlarm=false;
void sig_alarm(int iSig)
{
    bAlarm=true;
    alarm(1);
}
void sig_stop(int iSig)
{
    bStop=true;
}

int main(int argc,char *argv[])
{
    if(InitConf(CONF_FILE_PATH)!=0)
    {
        printf("init conf failed\n");
        return -1;
    }
    if(argc<2)
    {
        printf("<user msg>./mq_multi_consumer log_name\n");
        return -1;
    }
    int iRet = 0;
    char pLogPath[MQ_MAX_PATH_LEN]=DEFAULT_CLI_LOG_PATH;
    if(consumerLogger.Init(pLogPath)!=Logger::SUCCESS)
    {
        printf("logger init failed!");
        return -1;
    }
    //后台运行进程
    int ret=0;
    if((ret=FuncTool::DaemonInit())!=FuncTool::SUCCESS)
    {
        consumerLogger.WriteLog(mq_log_err,"Daemoninit faild!,return value is %d",ret);
        return -1;
    }
    Consumer *pConsumer=new Consumer();
    if(pConsumer->BuildConnection()!=Consumer::SUCCESS)
    {
        consumerLogger.WriteLog(mq_log_err,"Build Connection failed!errMsg is %s\n",pConsumer->GetErrMsg());
        delete pConsumer;
    }
    //创建队列，进行订阅
    if(pConsumer->CreateQueue("queue1",-1,false,false)!=Consumer::SUCCESS)
    {
        consumerLogger.WriteLog(mq_log_err,"Create queue failed!errMsg is %s\n",pConsumer->GetErrMsg());
    }
    //订阅接收,不确认
    if(pConsumer->CreateSubscribe("queue1",CONSUMER_NO_ACK)!=Consumer::SUCCESS)
    {
        consumerLogger.WriteLog(mq_log_err,"Create subscribe failed!errMsg is %s\n",pConsumer->GetErrMsg());
    }
    
    char pBuff[MAX_CLINT_PACKAGE_LENGTH];
    int iLen=MAX_CLINT_PACKAGE_LENGTH;
    //注册新号处理函数
    alarm(1);
    signal(SIGINT,sig_stop);
    signal(SIGTERM,sig_stop);
    signal(SIGALRM,sig_alarm);
    //循环进行消费
    int iLast=0;
    int i=0;
    int iSecond=0;
    while(!bStop)
    {
        memset(pBuff,0,MAX_CLINT_PACKAGE_LENGTH);
        ret=pConsumer->ConsumeMessage(pBuff,&iLen);
        if(ret!=Consumer::SUCCESS)
        {
            consumerLogger.WriteLog(mq_log_err,"Comsume message failed!errMsg is %s\n",pConsumer->GetErrMsg());
            break;
        }
        ++i;
        if(bAlarm)
        {
            bAlarm=false;
            int count=i-iLast;
            iLast=i;
            ++iSecond;
           consumerLogger.WriteLog(mq_log_info,"recv speed:%d/s\n",count);
        }
    }
    // if(iSecond!=0)
    // {
    //     LOG_INFO(0, 0,"average speed is %d/s\n",i/iSecond,i);
    // }

    if(pConsumer->DeleteQueue("queue1")!=Client::SUCCESS)
    {
        //printf("DeleteQueue failed!errMsg is %s\n",pConsumer->GetErrMsg());
    }
    delete pConsumer;
}