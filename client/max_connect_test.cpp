

//c/c++
#include<string.h>
#include<stdio.h>
#include<vector>
//linux

//user define
#include"../logger/logger.h"
#include"client.h"
using namespace WSMQ;
using std::vector;
//全局变量 
Logger producerLogger;

int main(int argc,char *argv[])
{
    if(InitConf(CONF_FILE_PATH)!=0)
    {
        printf("init conf failed\n");
        return -1;
    }
    char pLogPath[MQ_MAX_PATH_LEN]=DEFAULT_CLI_LOG_PATH;
    if(producerLogger.Init(pLogPath)!=Logger::SUCCESS)
    {
        printf("logger init failed!");
        return -1;
    }
    vector<Producer *>vProducers;
    int i=0;
    while(true)
    {
        Producer *pProducter=new Producer();
        vProducers.push_back(pProducter);
        if(pProducter->BuildConnection()!=Producer::SUCCESS)
        {
            producerLogger.WriteLog(mq_log_err,"Build Connection failed!errMsg is %s",pProducter->GetErrMsg());
            break;
        }
        ++i;
    }
    producerLogger.WriteLog(mq_log_info,"Build Connection count is %d",i);
    for(int i=0;i<vProducers.size();++i)
    {
        delete vProducers[i];
    }
}