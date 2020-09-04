
/**
 * @Descripttion: //写日志函数
 * @Author: readywang
 * @Date: 2020-07-22 14:15:50
 */
#include"stdio.h"

namespace WSMQ
{
    const static int MQ_MAX_PATH_LEN=100;
    #define DEFAULT_CLI_LOG_PATH "./mq_client_log/"
    #define DEFAULT_CONNECT_SERVER_LOG_PATH "./mq_conSrv_log/"
    #define DEFAULT_LOGIC_SERVER_LOG_PATH "./mq_logicSrv_log/"
    #define DEFAULT_PERSIS_SERVER_LOG_PATH "./mq_logicSrv_log/"
    #define ERR_LOG "ERR_LOG"
    #define WARN_LOG "WARN_LOG"
    #define INFO_LOG "INFO_LOG"
    typedef enum {mq_log_err=0,mq_log_warn,mq_log_info}mq_log_level;
    class Logger
    {
    public:
        const static int SUCCESS=0;
        const static int ERROR=-1;
        const static int ERR_LOG_INIT=-900;
    public:
        Logger();
        ~Logger();
        int Init(char pLogPath[MQ_MAX_PATH_LEN]);
        int WriteLog(mq_log_level iLevel,const char *pFmt,...);
        int Print(mq_log_level iLevel,const char *pFmt,...);
    protected:
        FILE *m_pLogFile;
        char m_pLogPath[MQ_MAX_PATH_LEN];
    };
}