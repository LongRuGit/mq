//c/c++
#include<string.h>
#include<stdio.h>

//linux
#include<unistd.h>
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
    Producer *pProducter=new Producer();
    if(pProducter->BuildConnection()!=Producer::SUCCESS)
    {
        printf("Build Connection failed!errMsg is %s\n",pProducter->GetErrMsg());
    }
    if(pProducter->CreateExchange("exchange1",EXCHANGE_TYPE_DIRECT)!=Producer::SUCCESS)
    {
        printf("Create exchange failed!errMsg is %s\n",pProducter->GetErrMsg());
    }
    if(pProducter->CreateQueue("queue3",10)!=Producer::SUCCESS)
    {
        printf("Create queue failed!errMsg is %s\n",pProducter->GetErrMsg());
    }
    if(pProducter->CreateBinding("exchange1","queue3","mq.priority.test")!=Producer::SUCCESS)
    {
        printf("Create binding failed!errMsg is %s\n",pProducter->GetErrMsg());
    }
    int i=1;
    char pVal[100];
    while(i<100)
    {
        memset(pVal,0,sizeof(pVal));
        sprintf(pVal,"%d",i);
        string strVal=pVal;
        string strMsg="priority message "+strVal;
        int ret=pProducter->PuslishMessage("exchange1","mq.priority.test",strMsg,i);
        if(ret!=Producer::SUCCESS)
        {
            break;
        }
        printf("send msg:%s,priority is %d\n",strMsg.c_str(),i);
        ++i;
    }
    if(pProducter->DeleteExchange("exchange1")!=Client::SUCCESS)
    {
        printf("Delete exchange failed!errMsg is %s\n",pProducter->GetErrMsg());
    }
    delete pProducter;
    pProducter=NULL;
}
