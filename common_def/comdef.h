

/**
 * @Descripttion: 网络包定义
 * @Author: readywang
 * @Date: 2020-07-20 09:11:13
 */

#ifndef INCLUDE_COMMON_DEFINE_H
#define INCLUDE_COMMON_DEFINE_H
namespace WSMQ
{
    //配置文件路径
    const static char *CONF_FILE_PATH="./mq_conf/mq_conf.ini";
    //客户端发上来的包的最大值
    const static int MAX_CLINT_PACKAGE_LENGTH = 1024*11;
    //服务器发送给客户端的包的最大长度
    const static int MAX_SERVER_PACKAGE_LENGTH = 1024*11;
    //服务器应用层缓冲区大小
    const static int SERVER_BUFFER_SIZE = 1024*100;
    //交换器名称队列名称和各种key的最大长度
    const static int MAX_NAME_LENGTH=32;
    //服务器默认IP
    const static char* SERVER_DEFAULT_IP_ADDR="100.96.188.244";
    //服务器默认端口
    const static unsigned short SERVER_DEFAULT_PORT=11111;
    //客户端最大连接数目
    const static int CLIENT_EPOLL_COUNT=1000;
    //读写文件时的缓冲区大小
    const static int MAX_RDWR_FILE_BUFF_SIZE=1024;
    //套接字发送缓冲区大小
    const static int SOCK_SEND_BUFF_SIZE=1024*1024 * 10;
    //套接字接收缓冲区大小
    const static int SOCK_RECV_BUFF_SIZE=1024*1024 * 10;
    //各种类型的消息的区分字段
    const static unsigned short CMD_CREATE_EXCNANGE=1;
    const static unsigned short CMD_CREATE_QUEUE=2;
    const static unsigned short CMD_CREATE_BINDING=3;
    const static unsigned short CMD_CREATE_PUBLISH=4;
    const static unsigned short CMD_CREATE_RECV=5;
    const static unsigned short CMD_CREATE_SUBCRIBE=6;
    const static unsigned short CMD_DELETE_EXCHANGE=7;
    const static unsigned short CMD_DELETE_QUEUE=8;
    const static unsigned short CMD_CANCEL_SUBCRIBE=9;
    const static unsigned short CMD_CLIENT_PULL_MESSAGE=10;
    const static unsigned short CMD_SERVER_PUSH_MESSAGE=11;
    const static unsigned short CMD_SERVER_REPLY_MESSAGE=12;
    const static unsigned short CMD_CLIENT_ACK_MESSAGE=13;
    const static unsigned short CMD_SERVER_ACK_MESSAGE=14;
    const static unsigned short CMD_CLIENT_EXIT=15;

    //exchange type 
    const static unsigned short EXCHANGE_TYPE_FANOUT=1;
    const static unsigned short EXCHANGE_TYPE_DIRECT=2;
    const static unsigned short EXCHANGE_TYPE_TOPIC=3;

    //确认级别
    const static  unsigned char PRODUCER_NO_ACK= 0;
    const static  unsigned char PRODUCER_SINGLE_ACK= 1;
    const static  unsigned char PRODUCER_MUTIL_ACK= 2;
    const static  unsigned char CONSUMER_NO_ACK= 0;
    const static  unsigned char CONSUMER_AUTO_ACK= 1;
    const static  unsigned char CONSUMER_SINGLE_ACK= 2;
    const static  unsigned char CONSUMER_MUTIL_ACK= 3;

    //接入层和业务逻辑层通信的共享内存的key
    const static int CONNECT_TO_LOGIC_KEY=12345;
    const static int LOGIC_TO_CONNECT_KEY=54321;

    //持久化层和业务逻辑层通信的共享内存的key
    const static int PERSIS_TO_LOGIC_KEY=23456;
    const static int LOGIC_TO_PERSIS_KEY=65432;

    //共享内存大小
    const static int SHM_QUEUE_SIZE=1024*1024*100;

    //持久化默认路径
    const static char *DEFAULT_EXCHANGE_PATH="./exchange/";
    const static char *DEFAULT_QUEUE_PATH="./queue/";
    //持久化message file默认大小
    const static int DURABLE_MESSAGE_FILE_SIZE=1024*1024*10;

    //定义网络包头部
    #pragma pack(1)
    typedef struct clientPackageHead_s
    {
        unsigned short m_iPackLen; //整个包长度
        int m_iClientIndex; //客户端在数组中的下标
        unsigned short m_iCmdId; //消息类型
    }ClientPackageHead;

    //网络包结构
    typedef struct client_package_s
    {
        ClientPackageHead m_ClientPackageHead;
        char *m_pPackageBody;
    }ClientPackage;
    #pragma pack()
}

#endif //INCLUDE_COMMON_DEFINE_H