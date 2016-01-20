#ifndef LOG_RLD
#define LOG_RLD

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 创建日期：2015-8-27
// 作    者：尹宾
// 修改日期：
// 修 改 者：
// 修改说明：
// 类 摘 要：日志库，包装了Log4cplus，对外提供统一的日志服务。
// 详细说明：
// 附加说明：
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <string>

#include <log4cplus/logger.h>   
#include <log4cplus/fileappender.h>   
#include <log4cplus/consoleappender.h>   
#include <log4cplus/layout.h> 
#include "log4cplus/loglevel.h"
#include "log4cplus/ndc.h"   
#include "log4cplus/configurator.h"  
#include "log4cplus/loggingmacros.h"

#include <list>


class LogRLD
{
public:
    LogRLD();
    ~LogRLD();

    static LogRLD &GetInstance();

    log4cplus::Logger *pLoggerImpl;

    bool Init(const int iLoglevel =LogRLD::INFO_LOG_LEVEL, const std::string &strHostName = "App(127.0.0.1)", 
        const std::string &strLogInnerShowName = "AppName", const std::string &strLogFilePath = "./App.log", 
        const int iSchedule = DAILY_LOG_SCHEDULE, const int iMaxLogFileBackupNum = 10);

    static const int OFF_LOG_LEVEL = log4cplus::OFF_LOG_LEVEL;

    static const int FATAL_LOG_LEVEL = log4cplus::FATAL_LOG_LEVEL;

    static const int ERROR_LOG_LEVEL = log4cplus::ERROR_LOG_LEVEL;

    static const int WARN_LOG_LEVEL = log4cplus::WARN_LOG_LEVEL;

    static const int INFO_LOG_LEVEL = log4cplus::INFO_LOG_LEVEL;

    static const int DEBUG_LOG_LEVEL = log4cplus::DEBUG_LOG_LEVEL;

    static const int TRACE_LOG_LEVEL = log4cplus::TRACE_LOG_LEVEL;

    static const int DAILY_LOG_SCHEDULE = log4cplus::DAILY;

    static const int HOURLY_LOG_SCHEDULE = log4cplus::HOURLY;
        
public:

    
private:
    static LogRLD *pLogRLD;

    



};

#define LOG_TRACE_RLD(p) LOG4CPLUS_TRACE(*LogRLD::GetInstance().pLoggerImpl, p)
#define LOG_DEBUG_RLD(p) LOG4CPLUS_DEBUG(*LogRLD::GetInstance().pLoggerImpl, p)
#define LOG_INFO_RLD(p) LOG4CPLUS_INFO(*LogRLD::GetInstance().pLoggerImpl, p)
#define LOG_WARN_RLD(p) LOG4CPLUS_WARN(*LogRLD::GetInstance().pLoggerImpl, p)
#define LOG_ERROR_RLD(p) LOG4CPLUS_ERROR(*LogRLD::GetInstance().pLoggerImpl, p)
#define LOG_FATAL_RLD(p) LOG4CPLUS_FATAL(*LogRLD::GetInstance().pLoggerImpl, p)
#define LOG_SYSERR_RLD(p) LOG4CPLUS_FATAL(*LogRLD::GetInstance().pLoggerImpl, p)
#define LOG_SYSFATAL_RLD(p) LOG4CPLUS_FATAL(*LogRLD::GetInstance().pLoggerImpl, p)

namespace log4cplus
{
    class DailyRollingFileAppenderExtend : public DailyRollingFileAppender
    {
    public:
        DailyRollingFileAppenderExtend(const log4cplus::tstring& filename,
            DailyRollingFileSchedule schedule = DAILY,
            bool immediateFlush = true,
            int maxBackupIndex = 10,
            bool createDirs = false);
        virtual ~DailyRollingFileAppenderExtend(){};

    protected:
        virtual void append(const spi::InternalLoggingEvent& event);
        void rolloverextend(const log4cplus::tstring &strScheduleBefore);

    private:
        std::list<log4cplus::tstring> m_strCurrentFileList;
    };
}


#endif