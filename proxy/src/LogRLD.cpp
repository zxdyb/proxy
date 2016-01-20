#include "LogRLD.h"
#include "log4cplus/spi/loggingevent.h"


LogRLD *LogRLD::pLogRLD = new LogRLD();

LogRLD::LogRLD() : pLoggerImpl(NULL)
{
}


LogRLD::~LogRLD()
{
}

LogRLD &LogRLD::GetInstance()
{
    return *pLogRLD;
}

bool LogRLD::Init(const int iLoglevel, const std::string &strHostName, const std::string &strLogInnerShowName, const std::string &strLogFilePath,
    const int iSchedule, const int iMaxLogFileBackupNum)
{
    log4cplus::DailyRollingFileSchedule schedulelog = (log4cplus::DailyRollingFileSchedule)iSchedule;
    

    /* step 1: Instantiate an appender object */
    log4cplus::SharedAppenderPtr _append(new log4cplus::DailyRollingFileAppenderExtend(strLogFilePath.c_str(), schedulelog, true, iMaxLogFileBackupNum));
    _append->setName("file log test");

    /* step 2: Instantiate a layout object */
    //std::string pattern = "%D{%Y%m%d %H:%M:%S.%Q} [%s-thread-%t ] %p   %s - %m - %l%n";
    std::string strPattern = "%D{%Y%m%d %H:%M:%S.%Q} [" + strHostName + "-thread-%T ] %p   " + strLogInnerShowName + " - %m - %l%n";
    

    std::auto_ptr<log4cplus::Layout> _layout(new log4cplus::PatternLayout(strPattern));

    /* step 3: Attach the layout object to the appender */
    _append->setLayout(_layout);

    /* step 4: Instantiate a logger object */
    pLoggerImpl = new log4cplus::Logger(log4cplus::Logger::getInstance("main_log"));
    //pLoggerImpl = log4cplus::Logger::getInstance("main_log");

    /* step 5: Attach the appender object to the logger  */
    pLoggerImpl->addAppender(_append);

    /* step 6: Set a priority for the logger  */
    pLoggerImpl->setLogLevel(iLoglevel);

    return true;
}


log4cplus::DailyRollingFileAppenderExtend::DailyRollingFileAppenderExtend(const log4cplus::tstring& filename, 
    DailyRollingFileSchedule schedule /*= DAILY*/, 
    bool immediateFlush /*= true*/, 
    int maxBackupIndex /*= 10*/, 
    bool createDirs /*= false*/) : DailyRollingFileAppender(filename, schedule, immediateFlush, maxBackupIndex, createDirs)
{

}

void log4cplus::DailyRollingFileAppenderExtend::append(const spi::InternalLoggingEvent& event)
{
    log4cplus::tstring strScheduleBefore;
    bool blNeed = false;
    if (event.getTimestamp() >= nextRolloverTime)
    {
        strScheduleBefore = scheduledFilename;
        blNeed = true;
    }

    DailyRollingFileAppender::append(event);

    if (blNeed)
    {
        rolloverextend(strScheduleBefore);
    }
}

void log4cplus::DailyRollingFileAppenderExtend::rolloverextend(const log4cplus::tstring &strScheduleBefore)
{
    //log4cplus::tstring strScheduleBefore = scheduledFilename;

    //DailyRollingFileAppender::rollover(alreadyLocked);
    
    if (m_strCurrentFileList.empty())
    {
        m_strCurrentFileList.push_back(strScheduleBefore);
    }
    else
    {
        if (strScheduleBefore != scheduledFilename)
        {
            m_strCurrentFileList.push_back(strScheduleBefore);

            if ((unsigned int)maxBackupIndex < m_strCurrentFileList.size())
            {
                log4cplus::tstring strToBeDelFile = m_strCurrentFileList.front();

                if (std::remove(strToBeDelFile.c_str()) != 0) //error
                {
                    printf("remove log file %s failed.\n", strToBeDelFile.c_str());
                }

                m_strCurrentFileList.pop_front();
            }
        }
    }
    
}
