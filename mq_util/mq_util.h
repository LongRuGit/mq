

/**
 * @Descripttion: mq中使用的公共函数
 * @Author: readywang
 * @Date: 2020-07-19 10:32:01
 */
#ifndef INCLUDE_MQ_UTIL_H
#define INCLUDE_MQ_UTIL_H

#include<sys/epoll.h>

namespace WSMQ
{
    class FuncTool
    {
    public:
        //错误信息定义
        const static int SUCCESS=0;   
        const static int ERROR=-1;
        const static int ERR_FUNC_TOOL_INIT_DAEMON=-301;
        const static int ERR_FUNC_TOOL_SET_NONBLOCK=-302;

        typedef union eight_bytes
        {
            __uint64_t u64;
            void *ptr;
        }EightBytes;

    public:
        static int DaemonInit();
        static int SetNonBlock(int iFd);
        static int MakeEpollEvent(struct epoll_event &iEvent,void *const ptr);
        static void *GetEventDataPtr(const struct epoll_event &iEvent);
        
        static int ReadBool(const void *ipBuffer, bool &ibVal);
        static int WriteBool(void *ipBuffer,bool ibVal);
        static int ReadByte(const void *ipBuffer, unsigned char &iVal);
        static int ReadByte(const void *ipBuffer, char &iVal);
        static int WriteByte(void *ipBuffer, unsigned char iVal);
        static int WriteByte(void *ipBuffer, char iVal);

        static int ReadShort(const void *ipBuffer, unsigned short &oVal, int iToHostOrder = 1);
        static int ReadShort(const void *ipBuffer,  short &oVal, int iToHostOrder = 1);
        static int WriteShort(void *ipBuffer, unsigned short iVal, int iToNetOrder = 1);
        static int WriteShort(void *ipBuffer, short iVal, int iToNetOrder = 1);

        static int ReadInt(const void *ipBuffer, unsigned int &iVal, int iToHostOrder = 1);
        static int ReadInt(const void *ipBuffer, int &iVal, int iToHostOrder = 1);
        static int WriteInt(void *ipBuffer, unsigned int iVal, int iToNetOrder = 1);
        static int WriteInt(void *ipBuffer, int iVal, int iToNetOrder = 1);
        static int ReadBuf(const void* ipSrc, void *iDest, int iLen);
        static int WriteBuf(void* ipDest, const void *ipSrc, int iLen);

        static int Sendn(int iSockfd,const void *ipBuffer,int iLen);
        static int Recvn(int iSockfd,void *opBuffer,int iLen);

        static const char *GetLogHeadTime(time_t tSec, time_t tUsec);

        static int CheckDir(const char * ipPath);
        static int MakeDir(const char * ipPath,bool bIsFilePath = false);
        static bool IsFileExist(const char * ipPath);
        static int RemoveDir(const char * ipPath);
        static int RemoveFile(const char * ipPath);
    };
}
#endif //INCLUDE_MQ_UTIL_H