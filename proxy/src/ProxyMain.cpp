// TestAll.cpp : 定义控制台应用程序的入口点。
//

//#ifdef _WIN32
//#include "stdafx.h"
//#include "../Storage/NetComm.h"
//#else
#include "NetComm.h"

//#endif

#include <list>
#include <string>
#include <map>
#include "ProxyServer.h"
#include "ProxyClient.h"
//#include<algorithm>

#include "LogRLD.h"
#include "ConfigSt.h"
#include <boost/filesystem.hpp> 
#include <boost/lexical_cast.hpp>

#define CONFIG_FILE_NAME "proxy.ini"

#ifdef _MSC_VER
#define strcasecmp _stricmp
#define strncasecmp  _strnicmp//strnicmp 
#endif


static void InitLog()
{
    std::string strHost = "Proxy(127.0.0.1)";
    std::string strLogPath = "./logs/";
    std::string strLogInnerShowName = "proxybin";
    int iLoglevel = LogRLD::INFO_LOG_LEVEL;
    int iSchedule = LogRLD::DAILY_LOG_SCHEDULE;
    int iMaxLogFileBackupNum = 10;
    
    boost::filesystem::path currentPath = boost::filesystem::current_path() / CONFIG_FILE_NAME;

    //判断配置文件是否存在
    if (boost::filesystem::exists(currentPath))
    {
        //初始化配置信息
        ConfigSt cfg(currentPath.string());
        std::string strHostCfg = cfg.GetItem("Log.Host");
        if (!strHostCfg.empty())
        {
            strHost = strHostCfg;
        }

        std::string strLogPathCfg = cfg.GetItem("Log.LogPath");
        if (!strLogPathCfg.empty())
        {
            strLogPath = strLogPathCfg;
        }

        std::string strLogLevelCfg = cfg.GetItem("Log.Level");
        if (!strLogLevelCfg.empty())
        {
            if (strncasecmp("TRACE", strLogLevelCfg.c_str(), 5) == 0)
            {
                iLoglevel = LogRLD::TRACE_LOG_LEVEL;
            }
            else if (strncasecmp("DEBUG", strLogLevelCfg.c_str(), 5) == 0)
            {
                iLoglevel = LogRLD::DEBUG_LOG_LEVEL;
            }
            else if (strncasecmp("INFO", strLogLevelCfg.c_str(), 4) == 0)
            {
                iLoglevel = LogRLD::INFO_LOG_LEVEL;
            }
            else if (strncasecmp("WARN", strLogLevelCfg.c_str(), 4) == 0)
            {
                iLoglevel = LogRLD::WARN_LOG_LEVEL;
            }
            else if (strncasecmp("ERROR", strLogLevelCfg.c_str(), 5) == 0)
            {
                iLoglevel = LogRLD::ERROR_LOG_LEVEL;
            }
            else if (strncasecmp("FATAL", strLogLevelCfg.c_str(), 5) == 0)
            {
                iLoglevel = LogRLD::FATAL_LOG_LEVEL;
            }
        }

        std::string strLogInnerShowNameCfg = cfg.GetItem("Log.LogFileName");
        if (!strLogInnerShowNameCfg.empty())
        {
            strLogInnerShowName = strLogInnerShowNameCfg;
        }

        std::string strScheduleCfg = cfg.GetItem("Log.Schedule");
        if (!strScheduleCfg.empty())
        {
            if (strncasecmp("HOURLY", strLogLevelCfg.c_str(), 6) == 0)
            {
                iSchedule = LogRLD::HOURLY_LOG_SCHEDULE;
            }
            else if (strncasecmp("DAILY", strLogLevelCfg.c_str(), 5) == 0)
            {
                iSchedule = LogRLD::DAILY_LOG_SCHEDULE;
            }
            else
            {

            }

        }

        std::string strMaxLogFileBackupNumCfg = cfg.GetItem("Log.FileNum");
        if (!strMaxLogFileBackupNumCfg.empty())
        {
            iMaxLogFileBackupNum = atoi(strMaxLogFileBackupNumCfg.c_str());
        }


    }

    boost::filesystem::path LogPath(strLogPath);
    std::string strFileName =strLogInnerShowName + ".log";
    LogPath = LogPath / strFileName;

    LogRLD::GetInstance().Init(iLoglevel, strHost, strLogInnerShowName, LogPath.string(), iSchedule, iMaxLogFileBackupNum);

}

static std::string GetConfig(const std::string &strItem)
{
    boost::filesystem::path currentPath = boost::filesystem::current_path() / CONFIG_FILE_NAME;

    //判断配置文件是否存在
    if (boost::filesystem::exists(currentPath))
    {
        //初始化配置信息
        ConfigSt cfg(currentPath.string());
        return cfg.GetItem(strItem);
    }
    return "";
}



int main(int argc, char* argv[])
{    
    
    InitLog();
    
    LOG_INFO_RLD("Proxy Server begin...");

    if (4 <= argc)
    {
        std::string strSrcIDBegin = argv[1];
        std::string strDstIDBegin = argv[2];
        unsigned int uiLinkNum = (unsigned int)atoi(argv[3]);


        std::string strMsgBodyLen = argv[4];
        transform(strMsgBodyLen.begin(), strMsgBodyLen.end(), strMsgBodyLen.begin(), tolower);        
        boost::int32_t iPos = 0;
        std::string strLen;
        boost::uint32_t uiLen = 0;
        //int ipos = std::string::npos;
        //int ifind = strMsgBodyLen.rfind('k');
        if ((boost::int32_t)std::string::npos != (iPos = strMsgBodyLen.rfind('k')))
        {
            strLen = strMsgBodyLen.substr(0, iPos);
            uiLen = (boost::uint32_t)atoi(strLen.c_str());
            uiLen = uiLen * 1024;
        }
        else if ((boost::int32_t)std::string::npos != (iPos = strMsgBodyLen.rfind('m')))
        {
            strLen = strMsgBodyLen.substr(0, iPos);
            uiLen = (boost::uint32_t)atoi(strLen.c_str());
            uiLen = uiLen * 1024 * 1024;
        }
        else
        {
            uiLen = (boost::uint32_t)atoi(strMsgBodyLen.c_str());
        }

        char *pIPAddress = argv[5];
        char *pIPPort = argv[6];
        char *pIsOnlyOneDst = argv[7];
        bool IsOnlyOneDst = false;
        if (std::string("true") == std::string(pIsOnlyOneDst))
        {
            IsOnlyOneDst = true;
        }
        else if (std::string("false") == std::string(pIsOnlyOneDst))
        {
            IsOnlyOneDst = false;
        }
        else
        {
            IsOnlyOneDst = false;
        }

        boost::uint32_t uiRunModule = ProxyClient::SEND;
        if (std::string("send") == std::string(argv[8]))
        {
            uiRunModule = ProxyClient::SEND;
        }
        else if (std::string("receive") == std::string(argv[8]))
        {
            uiRunModule = ProxyClient::RECEIVE;
        }
        else if (std::string("both") == std::string(argv[8]))
        {
            uiRunModule = ProxyClient::SEND | ProxyClient::RECEIVE;
        }

        printf("src id is %s, dst id is %s, only one dst is %s, link num is %u, msg len is %u, remote ip is %s, port is %s\n",
            strSrcIDBegin.c_str(), strDstIDBegin.c_str(), IsOnlyOneDst ? "true" : "false", uiLinkNum, uiLen, pIPAddress, pIPPort);

        
        //ProxyClient clt("23", "25", false, 1, 100, "172.16.23.181", "6666");
        ProxyClient clt(strSrcIDBegin, strDstIDBegin, IsOnlyOneDst, uiLinkNum, uiLen, pIPAddress, pIPPort);

        clt.Run(1, uiRunModule);

        //boost::this_thread::sleep(boost::posix_time::seconds(20));
    }
    else
    {
        unsigned short usPort = 6789;
        if (2 <= argc)
        {
            usPort = (unsigned short)atoi(argv[1]);            
        }

        unsigned int uiThreadNum = 2;
        if (3 <= argc)
        {
            uiThreadNum = (unsigned int)atoi(argv[2]);
        }

        LOG_INFO_RLD("Proxy runing thread number is " << uiThreadNum << " port is " << usPort);

        ProxyHub *phb = NULL;
        try
        {
            phb = new ProxyHub(usPort);
        }
        catch (...)
        {
            LOG_ERROR_RLD("Create proxy server failed, exit...");
            return 0;
        }

        const std::string &strSrcIDReplaceByIncSeq = GetConfig("General.SrcIDReplaceByIncSeq");
        bool blSrcIDReplaceByIncSeq = false;
        if (!strSrcIDReplaceByIncSeq.empty())
        {
            blSrcIDReplaceByIncSeq = boost::lexical_cast<bool>(strSrcIDReplaceByIncSeq);
        }
        phb->SetSrcIDReplaceByIncSeq(blSrcIDReplaceByIncSeq);

        bool blCreateSIDOnConnected = false;
        const std::string &strCreateSIDOnConnected = GetConfig("General.CreateSIDOnConnected");
        if (!strCreateSIDOnConnected.empty())
        {
            blCreateSIDOnConnected = boost::lexical_cast<bool>(strCreateSIDOnConnected);
        }
        phb->SetCreateSIDOnConnected(blCreateSIDOnConnected);

    
        LOG_INFO_RLD("Proxy begin running...");
        phb->Run(uiThreadNum);
    }

	return 0;
}

