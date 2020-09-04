//c/c++
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<string>
//linux
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<signal.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<sys/errno.h>
#include<time.h>
#include<dirent.h>
//user define
#include "mq_util.h"

using namespace WSMQ;

int FuncTool::DaemonInit()
{
    pid_t pid;
    if((pid=fork())==-1)
    {
        return ERR_FUNC_TOOL_INIT_DAEMON;
    }
    else if(pid>0)
    {
        exit(0);
    }
    if((pid=setsid())==-1)
    {
        return ERR_FUNC_TOOL_INIT_DAEMON;
    }
    signal(SIGHUP,SIG_IGN);
    signal(SIGPIPE,SIG_IGN);
    if((pid=fork())==-1)
    {
        return ERR_FUNC_TOOL_INIT_DAEMON;
    }
    else if(pid>0)
    {
        exit(0);
    }
    umask(0);
    return SUCCESS;
}

int FuncTool::SetNonBlock(int iFd)
{
    int flags=0;
    if((flags=fcntl(iFd,F_GETFL,0))<0||fcntl(iFd,F_SETFL,flags|O_NONBLOCK)<0)
    {
        return ERR_FUNC_TOOL_SET_NONBLOCK;
    }
    return SUCCESS;
}

int FuncTool::MakeEpollEvent(struct epoll_event &iEvent,void *const ptr)
{
    EightBytes temp;
    temp.ptr=ptr;
    iEvent.data.u64=temp.u64;
    return SUCCESS;
}

void *FuncTool::GetEventDataPtr(const struct epoll_event &iEvent)
{
    EightBytes temp;
    temp.u64=iEvent.data.u64;
    return temp.ptr;
}

int FuncTool::ReadBool(const void *ipBuffer, bool &ibVal)
{
    memcpy(&ibVal, ipBuffer, sizeof(bool));
    return sizeof(bool);
}

int FuncTool::WriteBool(void *ipBuffer,bool ibVal)
{
    memcpy(ipBuffer, &ibVal, sizeof(bool));
    return sizeof(bool);
}

int FuncTool::ReadByte(const void* ipBuffer, unsigned char &iVal)
{
    memcpy(&iVal, ipBuffer, sizeof(unsigned char));
    return sizeof(unsigned char);
}

int FuncTool::ReadByte(const void* ipBuffer, char &iVal)
{
    memcpy(&iVal, ipBuffer, sizeof(char));
    return sizeof(char);
}

int FuncTool::WriteByte(void* ipBuffer, unsigned char iVal)
{
    memcpy(ipBuffer, &iVal, sizeof(unsigned char));
    return sizeof(unsigned char);
}

int FuncTool::WriteByte(void* ipBuffer, char iVal)
{
    memcpy(ipBuffer, &iVal, sizeof(char));
    return sizeof(char);
}

int FuncTool::ReadShort(const void *ipBuffer, unsigned short &oVal, int iToHostOrder/* = 1*/)
{
    memcpy(&oVal, ipBuffer, sizeof(unsigned short));
    if (iToHostOrder == 1)
    {
        oVal = ntohs(oVal);
    }
    return sizeof(unsigned short);
}

int FuncTool::ReadShort(const void *ipBuffer,  short &oVal, int iToHostOrder/* = 1*/)
{
    memcpy(&oVal, ipBuffer, sizeof(unsigned short));
    if (iToHostOrder == 1)
    {
        oVal = ntohs(oVal);
    }
    return sizeof(unsigned short);
}

int FuncTool::WriteShort(void *ipBuffer, unsigned short iVal, int iToNetOrder/* = 1*/)
{
    if (iToNetOrder == 1)
    {
        iVal = htons(iVal);
    }
    memcpy(ipBuffer, &iVal, sizeof(unsigned short));
    return sizeof(unsigned short);
}

int FuncTool::WriteShort(void *ipBuffer, short iVal, int iToNetOrder/* = 1*/)
{
    if (iToNetOrder == 1)
    {
        iVal = htons(iVal);
    }
    memcpy(ipBuffer, &iVal, sizeof(short));
    return sizeof(short);
}


int FuncTool::ReadInt(const void *ipBuffer, unsigned int &iVal, int iToHostOrder/* = 1*/)
{
    memcpy(&iVal, ipBuffer, sizeof(unsigned int));
    if (iToHostOrder == 1)
    {
        iVal = ntohl(iVal);
    }
    return sizeof(unsigned int);
}

int FuncTool::ReadInt(const void *ipBuffer, int &iVal,  int iToHostOrder/* = 1*/)
{
    memcpy(&iVal, ipBuffer, sizeof(int));
    if (iToHostOrder == 1)
    {
        iVal = ntohl(iVal);
    }
    return sizeof(int);
}

int FuncTool::WriteInt(void *ipBuffer, unsigned int iVal,int iToNetOrder/* = 1*/)
{
    if (iToNetOrder == 1)
    {
        iVal = htonl(iVal);
    }
    memcpy(ipBuffer, &iVal, sizeof(unsigned int));
    return sizeof(unsigned int);
}

int FuncTool::WriteInt(void *ipBuffer, int iVal, int iToNetOrder/* = 1*/)
{
    if (iToNetOrder == 1)
    {
        iVal = htonl(iVal);
    }
    memcpy(ipBuffer, &iVal, sizeof(int));
    return sizeof(int);
}

int FuncTool::ReadBuf(const void* ipSrc, void *ipDest, int iLen)
{
    memcpy(ipDest, ipSrc, iLen);
    return iLen;
}

int FuncTool::WriteBuf(void* ipDest, const void *ipSrc, int iLen)
{
    memcpy(ipDest, ipSrc, iLen);
    return iLen;
}

int FuncTool::Sendn(int iSockfd,const void *ipBuffer,int iLen)
{
    int iLeft=iLen;
    const char *pTemp=(const char *)ipBuffer;
    while(iLeft>0)
    {
        int ret=send(iSockfd,pTemp,iLeft,0);
        if(ret<0)
        {
            if(errno==EWOULDBLOCK||errno==EAGAIN||errno==EINTR)
            {
                //套接字发送缓冲区无空闲空间，则继续等待
                continue;
            }
            return ERROR;
        }
        else if(ret==0)
        {
            return ERROR;
        }
        iLeft-=ret;
        pTemp+=ret;
    }
    return SUCCESS;
}

int FuncTool::Recvn(int iSockfd,void *opBuffer,int iLen)
{
    int iLeft=iLen;
    char *pTemp=(char *)opBuffer;
    int iRet=0;
    while(iLeft>0)
    {
        iRet=recv(iSockfd,pTemp,iLeft,0); 
        if(iRet<0)
        {
            if(errno==EWOULDBLOCK||errno==EAGAIN||errno==EINTR)
            {
                //套接字发送缓冲区无空闲空间，则继续等待
                continue;
            }
            return ERROR;
        }
        else if(iRet==0)
        {
            return ERROR;
        }
        iLeft-=iRet;
        pTemp+=iRet;
    }
    return SUCCESS;
}

const char *FuncTool::GetLogHeadTime(time_t tSec, time_t tUsec)
{
    static char szGetLogHeadTimeRet[64] = {0};
    struct tm *stSecTime = localtime(&tSec); //转换时间格式
    int year = 1900 + stSecTime->tm_year; //从1900年开始的年数
    int month = stSecTime->tm_mon + 1; //从0开始的月数
    int day = stSecTime->tm_mday; //从1开始的天数
    int hour = stSecTime->tm_hour; //从0开始的小时数
    int min = stSecTime->tm_min; //从0开始的分钟数
    int sec = stSecTime->tm_sec; //从0开始的秒数

    sprintf(szGetLogHeadTimeRet, "%s%04d-%02d-%02d %02d:%02d:%02d.%06ld",":",year, month, day, hour, min, sec, tUsec);
    return szGetLogHeadTimeRet;
}

int FuncTool::CheckDir(const char * ipPath)
{
    if (NULL == ipPath)
    {
        return ERROR;
    }

    umask(0);

    struct stat stBuf;
    if (lstat(ipPath, &stBuf) < 0)
    {
    	if(mkdir(ipPath, 0777) != -1)
    	{
    		return SUCCESS;
    	}
    }
    else
    {
    	return SUCCESS;
    }

    umask(0022);

    return SUCCESS;
}

int FuncTool::MakeDir(const char * ipPath,bool bIsFilePath/*=false*/)
{
	if (ipPath == NULL)
	{
		return ERROR;
	}

	int iLength = strlen(ipPath);

	if (iLength > 0)
	{
		char szDir[512] = { 0 };
		for (int i = 0; i < iLength; i++)
		{
			szDir[i] = ipPath[i];
			if (ipPath[i] == '/' && i != 0)
			{
				if (CheckDir(szDir) != SUCCESS)
				{
					return ERROR;
				}
			}
		}
		if (szDir[iLength - 1] != '/' && !bIsFilePath)
		{
			if (CheckDir(szDir) != SUCCESS)
			{
				return ERROR;
			}
		}
	}
	return SUCCESS;
}

bool FuncTool::IsFileExist(const char * ipPath)
{
    if (ipPath == NULL)
	{
		return ERROR;
	}
    struct stat buffer;
	return (stat(ipPath, &buffer) == 0);
}

int FuncTool::RemoveDir(const char * ipPath)
{
    DIR* pDir = opendir(ipPath);    
    if(!pDir)
    {
        return ERROR;
    }
    struct dirent *dir;
    struct stat st;
    while((dir = readdir(pDir)) != NULL)
    {
        if(strcmp(dir->d_name,".") == 0
                || strcmp(dir->d_name,"..") == 0)
        {
            continue;
        }    
        std::string sub_path = ipPath;
        sub_path=sub_path + "/" + dir->d_name;
        if(lstat(sub_path.c_str(),&st) == -1)
        {
            continue;
        }    
        if(S_ISDIR(st.st_mode))
        {
            if(RemoveDir(sub_path.c_str()) == -1) // 如果是目录文件，递归删除
            {
                closedir(pDir);
                return ERROR;
            }
            rmdir(sub_path.c_str());
        }
        else if(S_ISREG(st.st_mode))
        {
            unlink(sub_path.c_str());     // 如果是普通文件，则unlink
        }
        else
        {
            continue;
        }
    }
    if(rmdir(ipPath) == -1)//delete dir itself.
    {
        closedir(pDir);
        return ERROR;
    }
    closedir(pDir);
    return SUCCESS;
}

int FuncTool::RemoveFile(const char * ipPath)
{
    if(ipPath==NULL)
    {
        return ERROR;
    }
    struct stat st;    
    if(lstat(ipPath,&st) == -1)
    {
        return SUCCESS;
    }
    if(S_ISREG(st.st_mode))
    {
        if(unlink(ipPath) == -1)
        {
            return ERROR;
        }    
    }
    else if(S_ISDIR(st.st_mode))
    {
        if(strcmp(ipPath,".")==0 || strcmp(ipPath,"..")==0)
        {
            return ERROR;
        }    
        if(RemoveDir(ipPath) == -1)
        {
            return ERROR;
        }
    }
    return SUCCESS;
}