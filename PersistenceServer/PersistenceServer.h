

#ifndef INCLUDE_PERSISTENCE_H
#define INCLUDE_PERSISTENCE_H
#include<string>
#include<unordered_map>
#include<unordered_set>
#include<vector>
#include<list>
#include"../shm_queue/shm_queue.h"
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::list;
using std::vector;
namespace WSMQ
{
    class PersistenceServer
    {
    public:
        const static int SUCCESS=0;
        const static int ERROR=-1;
    public:
        static PersistenceServer*GetInstance();
        static void Destroy();
        int Init();
        int Run();
    protected:
        int InitConf(const char *ipPath);
        PersistenceServer();
        ~PersistenceServer();
        int InitSigHandler();
        static void SigTermHandler(int iSig){ m_bStop = true; }
        int OnCreateExchange(char *ipBuffer,int iLen);
        int OnCreateQueue(char *ipBuffer,int iLen);
        int OnDeleteExchange(char *ipBuffer,int iLen);
        int OnDeleteQueue(char *ipBuffer,int iLen);
        int OnSubcribe(char *ipBuffer,int iLen);
        int OnBinding(char *ipBuffer,int iLen);
        int OnPublishMessage(char *ipBuffer,int iLen,string istrQueueName,int iDurableIndex);
        int OnConsumeMessage(char *ipBuffer,int iLen);
        string ConvertIndexToString(int iIndex);
        //初始化文件信息
        int InitFileInfo();
        //找到文件夹中所有的index文件名称
        int FindIndexFiles(const char *ipDir,vector<string>&ovIndexFiles);
        //处理index文件
        int ProcessIndexFile(string istrQueueName,const char *ipFile);
        //清理文件，将连续的消费数目超过50%的文件进行合并
        int ClearUpFile(int &oClearCount);
        //计算该组文件消费比率
        double CalculateCondsumeRate(const char *ipFile);
        //合并文件
        int MergeFiles(const char *ipQueueName,const char *ipFileName1,const char *ipFileName2);
    protected:
        unordered_map<string,string>m_mMsgQueueTailFile;//每个队列当前写入文件
        unordered_map<string,unordered_map<int,string>>m_mMsgQueueIndexFile; //每个队列消费index对应文件
        unordered_map<string,list<string>>m_mMsgQueueList; //每个队列所有组文件名链表，从小到大排序
        ShmQueue *m_pQueueFromLogic;
        ShmQueue *m_pQueueToLogic;
        static bool m_bStop;
        static PersistenceServer* m_pPersistenceServer;
    };
    bool PersistenceServer::m_bStop=true;
    PersistenceServer * PersistenceServer::m_pPersistenceServer=NULL;
}

#endif //INCLUDE_PERSISTENCE_H
