
//c/c++
#include<string.h>
#include<stdio.h>

//linux

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
    if(pProducter->CreateExchange("exchange1",EXCHANGE_TYPE_DIRECT,true,false)!=Producer::SUCCESS)
    {
        printf("Create exchange failed!errMsg is %s\n",pProducter->GetErrMsg());
    }
    if(pProducter->CreateQueue("queue4",-1,true,false)!=Producer::SUCCESS)
    {
        printf("Create queue failed!errMsg is %s\n",pProducter->GetErrMsg());
    }
    if(pProducter->CreateBinding("exchange1","queue4","mq.durable.test")!=Producer::SUCCESS)
    {
        printf("Create binding failed!errMsg is %s\n",pProducter->GetErrMsg());
    }
    int i=1;
    char pVal[100];
    while(i<10000)
    {
        memset(pVal,0,sizeof(pVal));
        sprintf(pVal,"%d",i);
        string strVal=pVal;
        string strMsg="durable message "+strVal;
        int ret=pProducter->PuslishMessage("exchange1","mq.durable.test",strMsg,-1,true);
        if(ret!=Producer::SUCCESS)
        {
            break;
        }
        printf("send msg:%s\n",strMsg.c_str());
        ++i;
    }
    // if(pProducter->DeleteExchange("exchange1")!=Client::SUCCESS)
    // {
    //     printf("Delete exchange failed!errMsg is %s",pProducter->GetErrMsg());
    // }
    delete pProducter;
    pProducter=NULL;
}
