//c/c++
#include<string.h>
#include<stdio.h>

//linux
#include<poll.h>
//user define
#include"client.h"

using namespace WSMQ;

int main(int argc,char *argv[])
{
    if(InitConf(CONF_FILE_PATH)!=0)
    {
        printf("init conf failed\n");
        return -1;
    }
    
    char pBuff[MAX_CLINT_PACKAGE_LENGTH];
    int iLen=MAX_CLINT_PACKAGE_LENGTH;
    Consumer *pConsumer=new Consumer();
    if(pConsumer->BuildConnection()!=Consumer::SUCCESS)
    {
        printf("Build Connection failed!errMsg is %s\n",pConsumer->GetErrMsg());
    }
    //创建队列
    if(pConsumer->CreateQueue("queue4")!=Consumer::SUCCESS)
    {
        printf("Create queue failed!errMsg is %s\n",pConsumer->GetErrMsg());
    }
    //订阅接收,自动确认
    if(pConsumer->CreateSubscribe("queue4",CONSUMER_NO_ACK)!=Consumer::SUCCESS)
    {
        printf("Create subscribe failed!errMsg is %s\n",pConsumer->GetErrMsg());
    }
    int ret=0;
    while(true)
    {
        memset(pBuff,0,MAX_CLINT_PACKAGE_LENGTH);
        ret=pConsumer->ConsumeMessage(pBuff,&iLen);
        if(ret!=Consumer::SUCCESS)
        {
            printf("Comsume message failed!errMsg is %s\n",pConsumer->GetErrMsg());
            break;
        }
        printf("recv msg :%s\n",pBuff);
    }
    if(pConsumer->CancelSubscribe("queue4")!=Client::SUCCESS)
    {
        printf("CancelSubscribe failed!errMsg is %s\n",pConsumer->GetErrMsg());
    }
    if(pConsumer->DeleteQueue("queue4")!=Client::SUCCESS)
    {
        printf("DeleteQueue failed!errMsg is %s\n",pConsumer->GetErrMsg());
    }

    delete pConsumer;
    pConsumer=NULL;
}