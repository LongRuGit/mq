/**定义客户端发送的每一种消息类型
 * @Descripttion: 
 * @Author: readywang
 * @Date: 2020-07-20 15:14:03
 */

#ifndef INCLUDE_MESSAGE_H
#define INCLUDE_MESSAGE_H

#include"../common_def/comdef.h"
#include<string>
#include<sys/time.h>
using std::string;

namespace WSMQ
{
    class Message
    {
    public:
        const static int SUCCESS=0;
        const static int ERROR=-1;
        const static int ERR_MSG_NAME_TOO_LONG=-500;
        const static int ERR_MSG_BUF_TOO_SMALL=-501;
    public:
        Message(unsigned short iCmd):m_iIndex(-1),m_iCmdId(iCmd){}
        virtual ~Message(){};
        virtual int GetMessagePack(char *ipBuf,int *iopBuffLen)=0;
        void SetClientIndex(int iIndex){m_iIndex=iIndex;}
        int GetClientIndex(){return m_iIndex;}
        void SetCmdId(unsigned short iId){m_iCmdId=iId;}
        unsigned short GetCmdId(){return m_iCmdId;}
    protected:
        int m_iIndex;
        unsigned short m_iCmdId;
    };

    class CreateExchangeMessage :public Message
    {
    public:
        CreateExchangeMessage(const string &istrName,unsigned short iExchangeType,bool ibDurable=false,bool ibAutoDel=true);
        string GetExchangeName(){return m_strExchangeName;}
        void SetExchangeName(const string& istrName){m_strExchangeName=istrName;}
        unsigned short GetExchangetype(){return m_iExchangeType;}
        void SetExchangetype(int iType){m_iExchangeType=iType;}
        bool IsDurable(){return m_bDurable==true;}
        bool IsAutoDel(){return m_bAutoDel==true;}
        int GetMessagePack(char *ipBuf,int *iopBuffLen);
    private:
        string m_strExchangeName;
        unsigned short m_iExchangeType;
        bool m_bDurable;
        bool m_bAutoDel;
    };

    class CreateQueueMessage :public Message
    {
    public:
        CreateQueueMessage(const string &istrName,short iPriority=-1,bool ibDurable=false,bool ibAutoDel=true);
        string GetQueueName(){return m_strQueueName;}
        void SetQueueName(const string &istrName){m_strQueueName=istrName;}
        short GetPriority(){return m_iPriority;}
        void SetPriority(short iPriority){m_iPriority=iPriority;}
        bool IsDurable(){return m_bDurable==true;}
        bool IsAutoDel(){return m_bAutoDel==true;}
        int GetMessagePack(char *ipBuf,int *iopBuffLen);
    protected:
        string m_strQueueName;
        short m_iPriority;
        bool m_bDurable;
        bool m_bAutoDel;
    };

    class CreateBindingMessage :public Message
    {
    public:
        CreateBindingMessage(const string &istrExName, const string &istrQueueName, const string &istrKey);
        string GetExchangeName(){return m_strExchangeName;}
        void SetExchangeName(const string &istrName){m_strExchangeName=istrName;}
        string GetQueueName(){return m_strQueueName;}
        void SetQueueName(const string &istrName){m_strQueueName=istrName;}
        string GetBindingKey(){return m_strBindingKey;}
        void SetBindingKey(const string &istrKey){m_strBindingKey=istrKey;}
        int GetMessagePack(char *ipBuf,int *iopBuffLen);
    protected:
        string m_strExchangeName;
        string m_strQueueName;
        string m_strBindingKey;
    };

    class CreatePublishMessage:public Message
    {
    public:
        CreatePublishMessage(const string &istrExName,const string &istrKey,const string &istrMsgBody,
        short iPriority=-1,bool ibDurable=false,unsigned char iConfirmLevel=0);
        string GetExchangeName(){return m_strExchangeName;}
        void SetExchangeName(const string& istrName){m_strExchangeName=istrName;}
        string GetRoutingKey(){return m_strRoutingKey;}
        void SetRoutingKey(const string &istrKey){m_strRoutingKey=istrKey;}
        short GetPriority(){return m_iPriority;}
        void SetPriority(short iPriority){m_iPriority=iPriority;}
        bool IsDurable(){return m_bDurable==true;}
        void SetMsgSeq(int iMsgSeq){m_iMsgSeq=iMsgSeq;}
        int GetMsgSeq(){return m_iMsgSeq;}
        void SetConfirmLevel(unsigned char iConfirmLevel){m_iConfirmLevel=iConfirmLevel;}
        int GetConfirmLevel(){return m_iConfirmLevel;}
        void SetSaveTime(){gettimeofday(&m_tSaveTime,NULL);}
        struct timeval GetSaveTime(){return m_tSaveTime;}
        string GetMsgBody(){return m_strMsgBody;}
        void SetMsgBody(const string &istrBody){m_strMsgBody=istrBody;}
        int GetMessagePack(char *ipBuf,int *iopBuffLen);
    protected:
        string m_strExchangeName;
        string m_strRoutingKey;
        short m_iPriority;
        bool m_bDurable;
        int m_iMsgSeq;
        unsigned char m_iConfirmLevel;
        string m_strMsgBody;
        struct timeval m_tSaveTime;
    };

    class CreateRecvMessage:public Message
    {
    public:
        CreateRecvMessage(const string &istrQueueName,unsigned char iConfirmLevel=0);
        string GetQueueName(){return m_strQueueName;}
        void SetQueueName(const string &istrName){m_strQueueName=istrName;}
        void SetConfirmLevel(unsigned char iConfirmLevel){m_iConfirmLevel=iConfirmLevel;}
        int GetConfirmLevel(){return m_iConfirmLevel;}
        int GetMessagePack(char *ipBuf,int *iopBuffLen);
    protected:
        string m_strQueueName;
        unsigned char m_iConfirmLevel;
    };

    class CreateSubscribeMessage:public Message
    {
    public:
        CreateSubscribeMessage(const string &istrQueueName,unsigned char iConfirmLevel=0);
        string GetQueueName(){return m_strQueueName;}
        void SetQueueName(const string &istrName){m_strQueueName=istrName;}
        void SetConfirmLevel(unsigned char iConfirmLevel){m_iConfirmLevel=iConfirmLevel;}
        int GetConfirmLevel(){return m_iConfirmLevel;}
        int GetMessagePack(char *ipBuf,int *iopBuffLen);
    protected:
        string m_strQueueName;
        unsigned char m_iConfirmLevel;
    };

    class DeleteExchangeMessage:public Message
    {
    public:
        DeleteExchangeMessage(const string &istrExName):Message(CMD_DELETE_EXCHANGE),m_strExchangeName(istrExName){}
        string GetExchangeName(){return m_strExchangeName;}
        void SetExchangeName(const string& istrName){m_strExchangeName=istrName;}
        int GetMessagePack(char *ipBuf,int *iopBuffLen);
    protected:
        string m_strExchangeName;
    };

    class DeleteQueueMessage:public Message
    {
    public:
        DeleteQueueMessage(const string &istrQueueName):Message(CMD_DELETE_QUEUE),m_strQueueName(istrQueueName){}
        string GetQueueName(){return m_strQueueName;}
        void SetQueueName(const string &istrName){m_strQueueName=istrName;}
        int GetMessagePack(char *ipBuf,int *iopBuffLen);
    protected:
        string m_strQueueName;
    };

    class CancelSubscribeMessage:public Message
    {
    public:
        CancelSubscribeMessage(const string &istrQueueName):Message(CMD_CANCEL_SUBCRIBE),m_strQueueName(istrQueueName){}
        string GetQueueName(){return m_strQueueName;}
        void SetQueueName(const string &istrName){m_strQueueName=istrName;}
        int GetMessagePack(char *ipBuf,int *iopBuffLen);
    protected:
        string m_strQueueName;
    };

    class SeverStoreMessage:public Message
    {
    public:
        SeverStoreMessage(const string &istrMsgBody, short iPriority=-1,bool ibDurable=false);
        string GetMsgBody(){return m_strMsgBody;}
        void SetMsgBody(const string &istrBody){m_strMsgBody=istrBody;}
        short GetPriority(){return m_iPriority;}
        void SetPriority(short iPriority){m_iPriority=iPriority;}
        void SetConfirmLevel(unsigned char iConfirmLevel){m_iConfirmLevel=iConfirmLevel;}
        int GetConfirmLevel(){return m_iConfirmLevel;}
        int GetMsgSeq(){return m_iMsgSeq;}
        void SetMsgSeq(int iMsgSeq){m_iMsgSeq=iMsgSeq;}
        int GetDurableIndex(){return m_iDurableIndex;}
        void SetDurableIndex(int iDurableIndex){m_iDurableIndex=iDurableIndex;}
        bool IsDurable(){return m_bDurable==true;}
        string GetBelongQueueName(){return m_strQueueName;}
        void SetBelongQueueName(string istrName){m_strQueueName=istrName;}
        void SetSaveTime(){gettimeofday(&m_tSaveTime,NULL);}
        struct timeval GetSaveTime(){return m_tSaveTime;}
        int GetMessagePack(char *ipBuf,int *iopBuffLen);
        int SerializeDurableToString(char *ipBuffer,int &iBuffLen);
    protected:
        short m_iPriority;
        bool m_bDurable;
        unsigned char m_iConfirmLevel;
        int m_iMsgSeq;
        int m_iDurableIndex;
        string m_strMsgBody;
        string m_strQueueName;
        struct timeval m_tSaveTime;
    };

    class ActReplyMessage:public Message
    {
    public:
        ActReplyMessage(bool ibSucceed,const string &istrMsgBody);
        int GetMessagePack(char *ipBuf,int *iopBuffLen);
    protected:
        bool m_bSucceed;
        string m_strMsgBody;
    };

        class ClientAckMessage:public Message
    {
    public:
        ClientAckMessage(unsigned char iConfirmLevel,int iAckSeq);
        void SetConfirmLevel(unsigned char iConfirmLevel){m_iConfirmLevel=iConfirmLevel;}
        int GetConfirmLevel(){return m_iConfirmLevel;}
        unsigned short GetAckSeq(){return m_iAckSeq;}
        void SetMsgSeq(unsigned short iAckSeq){m_iAckSeq=iAckSeq;}
        int GetMessagePack(char *ipBuf,int *iopBuffLen);
    protected:
        unsigned char m_iConfirmLevel;
        int m_iAckSeq;
    };

    class ServerAckMessage:public Message
    {
    public:
        ServerAckMessage(int iAckSeq);
        unsigned short GetAckSeq(){return m_iAckSeq;}
        void SetMsgSeq(unsigned short iAckSeq){m_iAckSeq=iAckSeq;}
        int GetMessagePack(char *ipBuf,int *iopBuffLen);
    protected:
        int m_iAckSeq;
    };
    
    class ClientExitMessage:public Message
    {
    public:
        ClientExitMessage(int iClientIndex);
        int GetMessagePack(char *ipBuf,int *iopBuffLen);
    };
}
#endif //INCLUDE_MESSAGE_H
