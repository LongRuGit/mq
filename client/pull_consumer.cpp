

//c/c++
#include<string.h>
#include<stdio.h>

//linux

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
    if(pConsumer->CreateQueue("queue5")!=Consumer::SUCCESS)
    {
       consumerLogger.WriteLog(mq_log_err,"Create queue failed!errMsg is %s",pConsumer->GetErrMsg());
    }
    int ret=0; 
    int i=1;
    while(true)
    {
        memset(pBuff,0,MAX_CLINT_PACKAGE_LENGTH);
        ret=pConsumer->RecvMessage("queue5",pBuff,&iLen,true);
        if(ret!=Consumer::SUCCESS)
        {
            // LOG_INFO(0, 0,"recv message failed!errMsg is %s",pConsumer->GetErrMsg());
            // consumerLogger.Print(mq_log_err,"recv message failed!errMsg is %s",pConsumer->GetErrMsg());
        }
        else
        {
            if(i%1000==0)
            {
                consumerLogger.WriteLog(mq_log_info,"recv msg :%s\n",pBuff);
            }
            ++i;
        }  
    }

    if(pConsumer->DeleteQueue("queue5")!=Client::SUCCESS)
    {
        consumerLogger.WriteLog(mq_log_err,"DeleteQueue failed!errMsg is %s",pConsumer->GetErrMsg());
    }
    delete pConsumer;
    pConsumer=NULL;
}