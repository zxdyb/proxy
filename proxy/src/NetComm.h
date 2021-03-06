#ifndef NET_COMM
#define NET_COMM
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 创建日期：2014-11-20
// 作    者：尹宾
// 修改日期：
// 修 改 者：
// 修改说明：
// 类 摘 要：通讯Client端
// 详细说明：
// 附加说明：
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <string>
#include <list>
#include <boost/atomic/atomic.hpp>
#include <boost/lockfree/queue.hpp> 
#include <unordered_map>

typedef boost::function<void()>RunCallFunc;

class UDPClient : public boost::noncopyable
{
public:
    UDPClient(const char *pIPAddress, const boost::uint16_t uiPort);
    ~UDPClient(void);

    bool SendMessageSyn(const char *pMessage, const boost::uint32_t uiSize);

private:
    boost::asio::io_service m_ioservice;
    boost::asio::ip::address m_address;
    boost::asio::ip::udp::endpoint m_endpoint;
    boost::asio::ip::udp::socket m_socket;

    bool m_InitSucceed;
};

class UDPServer : public boost::noncopyable
{
public:
    UDPServer(const boost::uint16_t uiPort);
    ~UDPServer();

    bool ReceiveMessageSyn(char *pMessage, const boost::uint32_t uiSize);

private:
    boost::asio::io_service m_ioservice;
    boost::asio::ip::udp::socket m_socket;
    
};

//////////////////////////////////////////////////////////////////////////////

// echo_server.cpp
// g++ -o echo_server -O3 echo_server.cpp -lboost_system -lboost_thread
class TCPSessionOfServer;
typedef boost::function<void(boost::shared_ptr<TCPSessionOfServer>, const boost::system::error_code&)> AcceptedCallback;
typedef boost::function<void(boost::shared_ptr<TCPSessionOfServer>, const boost::system::error_code&, std::size_t, void *)> ServerReadOrWriteCallback;
typedef boost::function<std::size_t(const boost::system::error_code& error, std::size_t bytes_transferred)> ConditionCB;
typedef boost::function<bool(boost::shared_ptr<TCPSessionOfServer>)> TimeoutCB;



class TCPSessionOfServer
	: public boost::enable_shared_from_this<TCPSessionOfServer>, public boost::noncopyable
{
public:
    TCPSessionOfServer(boost::asio::io_service &TMIOService, boost::asio::io_service& io_service, 
        ServerReadOrWriteCallback RdCB = NULL, ServerReadOrWriteCallback WtCB = NULL);

    ~TCPSessionOfServer();

    void SetCallBack(ServerReadOrWriteCallback cb, const bool IsReadCB);

    void SetTimeoutCB(TimeoutCB tcb, const bool IsReadCB);

	boost::asio::ip::tcp::socket& GetSocket();

    void AsyncWrite(char *pInputBuffer, const boost::uint32_t uiBufferSize, const boost::uint32_t uiWtTimeOutSec = 0,
        void *pValue = NULL);

    void AsyncRead(char *pOutputBuffer, const boost::uint32_t uiBufferSize, const boost::uint32_t uiRdTimeOutSec = 0,
        void *pValue = NULL, const boost::uint32_t uiNeedReadSize = 0);

    bool SyncWrite(char *pInputBuffer, const boost::uint32_t uiBufferSize, boost::uint32_t &uiSizeofWrited,
        std::string &strErrMsg);

    void HandleWtTimeOut(boost::shared_ptr<boost::asio::deadline_timer> pTimer, const boost::uint32_t uiWtTimeOutSec, const boost::system::error_code& e);

    void HandleRdTimeOut(boost::shared_ptr<boost::asio::deadline_timer> pTimer, const boost::uint32_t uiRdTimeOutSec, const boost::system::error_code& e);

    void Close();
    
private:
    void SocketCancel(const boost::system::error_code& e);

    void HandleRead(boost::shared_ptr<boost::asio::deadline_timer> pTimer, const boost::system::error_code& error,
        std::size_t bytes_transferred, void *pValue);

    void HandleWrite(boost::shared_ptr<boost::asio::deadline_timer> pTimer, const boost::system::error_code& error,
        std::size_t bytes_transferred, void *pValue);

private:
    boost::asio::io_service &m_TMIOService;
	boost::asio::ip::tcp::socket m_Socket;

	ServerReadOrWriteCallback m_RdCB;
	ServerReadOrWriteCallback m_WtCB;
    TimeoutCB m_RdTcb;
    TimeoutCB m_WtTcb;

    boost::mutex m_Mutex;
};

class TCPServer : public boost::noncopyable
{
public:
	TCPServer(unsigned short port);

	void Run(const boost::uint32_t uiThreadNum, AcceptedCallback ApBck, ServerReadOrWriteCallback RdBck, ServerReadOrWriteCallback WtBck,
		const bool isWaitRunFinished = false);

    boost::asio::io_service &GetIOService()
    {
        return m_IOService;
    };
private:
    void RunTimeIOService();

    void RunIOService();

	void StartAccept(ServerReadOrWriteCallback RdBck = NULL, ServerReadOrWriteCallback WtBck = NULL);

	void HandleAccept(boost::shared_ptr<TCPSessionOfServer> new_session,
		const boost::system::error_code& error, ServerReadOrWriteCallback RdBck, ServerReadOrWriteCallback WtBck);

    void RunCommonIOService(boost::asio::io_service *pIoSvr);

	boost::asio::io_service m_IOService;
    boost::asio::io_service m_IOServiceAcceptor;
	boost::asio::ip::tcp::acceptor m_Acceptor;
	boost::asio::io_service::work m_IOWork;
    boost::asio::io_service::work m_IOWorkAcceptor;
	boost::thread_group m_RunThdGrp;
	AcceptedCallback m_AcceptedCallback;

    boost::atomic_uint32_t m_SessionNum;
    struct SessionIOService
    {
        SessionIOService() : m_IOService(new boost::asio::io_service), 
            m_IOWork(new boost::asio::io_service::work(*m_IOService)) {};
        boost::asio::io_service *m_IOService;
        boost::asio::io_service::work *m_IOWork;
    };
    std::list<SessionIOService> m_SessionIOServiceList;
    static const boost::uint32_t SESSION_LIMIT_NUM = 50;

    boost::asio::io_service m_TimeIOService;
    boost::asio::io_service::work m_TimeIOWork;
};


////////////////////////////////////////////////////////////////////////////////
// g++ -o echo_client -O3 echo_client.cpp -lboost_system -lboost_thread
//////////////////////////////////////////////////////////////////////////////
typedef boost::function<void(const boost::system::error_code&)> ConnectedCallback;
typedef boost::function<void(const boost::system::error_code&, std::size_t, void *)> ReadOrWriteCallback;

////////////////////////////////////////////////////////////////////////////////

class TCPSessionOfClient : public boost::enable_shared_from_this<TCPSessionOfClient>, public boost::noncopyable
{
public:
    TCPSessionOfClient(boost::asio::io_service &TMIOService, boost::asio::io_service& io_service, ReadOrWriteCallback RdBck, ReadOrWriteCallback WtBck,
        const boost::uint32_t uiRdTimeOutSec, const boost::uint32_t uiWtTimeOutSec);

    boost::asio::ip::tcp::socket& GetSocket();

    void HandleWrite(boost::shared_ptr<boost::asio::deadline_timer> pTimer, const boost::system::error_code& ec, 
        std::size_t bytes_transfered, void *pValue);

    void HandleRead(boost::shared_ptr<boost::asio::deadline_timer> pTimer, const boost::system::error_code& ec, 
        std::size_t bytes_transfered, void *pValue);

    void AsyncWrite(char *pInputBuffer, const boost::uint32_t uiBufferSize, const boost::uint32_t uiWtTimeOutSec,
                            void *pValue = NULL);

    void AsyncRead(char *pOutputBuffer, const boost::uint32_t uiBufferSize, const boost::uint32_t uiRdTimeOutSec,
                            void *pValue = NULL, const boost::uint32_t uiNeedReadSize = 0);

    bool SyncWrite(char *pInputBuffer, const boost::uint32_t uiBufferSize, boost::uint32_t &uiSizeofWrited,
        std::string &strErrMsg);

    bool SyncRead(char *pOutputBuffer, const boost::uint32_t uiBufferSize, boost::uint32_t &uiSizeofReaded, std::string &strErrMsg);

    void HandleWtTimeOut(const boost::system::error_code& e);

    void HandleRdTimeOut(const boost::system::error_code& e);

    void Close();

private:
    void SocketCancel(const boost::system::error_code& e);

private:
    boost::asio::io_service &m_TMIOService;
    boost::asio::ip::tcp::socket m_Socket;

    ReadOrWriteCallback m_RdBck;
    ReadOrWriteCallback m_WtBck;

    boost::uint32_t m_uiRdTimeOutSec;
    boost::uint32_t m_uiWtTimeOutSec;

    boost::mutex m_Mutex;

};

class TCPClient : public boost::noncopyable
{
public:
    TCPClient(const char *pIPAddress, const char *pIPPort, ConnectedCallback CnBck, ReadOrWriteCallback RdBck,
        ReadOrWriteCallback WtBck, 
        const boost::uint32_t uiCnTimeOutSec = 0, 
        const boost::uint32_t uiRdTimeOutSec = 0,
        const boost::uint32_t uiWtTimeOutSec = 0);

    TCPClient(const char *pIPAddress, const char *pIPPort);

    TCPClient(const char *pIPAddress, const char *pIPPort, boost::asio::io_service &IOService, 
        boost::asio::io_service::work &IOWork);


    void Run(const boost::uint32_t uiThreadNum, bool isWaitRunFinished = false);

    void Run(const boost::uint32_t uiThreadNum, ConnectedCallback CnBck, ReadOrWriteCallback RdBck,
                    ReadOrWriteCallback WtBck, bool isWaitRunFinished = false,
                    const boost::uint32_t uiCnTimeOutSec = 0, 
                    const boost::uint32_t uiRdTimeOutSec = 0,
                    const boost::uint32_t uiWtTimeOutSec = 0);

    void SetCallBack(ConnectedCallback CnBck, ReadOrWriteCallback RdBck, ReadOrWriteCallback WtBck, 
        const boost::uint32_t uiCnTimeOutSec = 0,
        const boost::uint32_t uiRdTimeOutSec = 0,
        const boost::uint32_t uiWtTimeOutSec = 0);

    void Close();

    void Stop();

    void AsyncWrite(char *pInputBuffer, const boost::uint32_t uiBufferSize, 
        const boost::uint32_t uiWtTimeOutSec = 0, void *pValue = NULL);

    void AsyncRead(char *pOutputBuffer, const boost::uint32_t uiBufferSize, 
        const boost::uint32_t uiRdTimeOutSec = 0, void *pValue = NULL, const boost::uint32_t uiNeedReadSize = 0);

    void AsyncConnect(const boost::uint32_t uiCnTimeOutSec = 0);

    bool SyncWrite(char *pInputBuffer, const boost::uint32_t uiBufferSize, boost::uint32_t &uiSizeofWrited,
        std::string &strErrMsg);

    bool SyncRead(char *pOutputBuffer, const boost::uint32_t uiBufferSize, boost::uint32_t &uiSizeofReaded, std::string &strErrMsg);

    void Post(RunCallFunc func);
    
private:
    void RunIOService();
    void HandleConnect(boost::shared_ptr<TCPSessionOfClient> session_ptr, const boost::system::error_code& ec);

    void HandleCnTimeOut(const boost::system::error_code& e);

    void SocketCancel(const boost::system::error_code& e);

    void RunTimeIOService();

private:
    std::string m_strIPAddress;
    std::string m_strIPPort;

    ConnectedCallback m_CnBck;
    boost::asio::io_service &m_IOService;
    boost::thread_group m_RunThdGrp;
    boost::shared_ptr<TCPSessionOfClient> m_Session;    
    boost::asio::io_service::work &m_IOWork;

    boost::uint32_t m_uiCnTimeOutSec;
    boost::uint32_t m_uiRdTimeOutSec;
    boost::uint32_t m_uiWtTimeOutSec;

    boost::asio::io_service m_TimeIOService;
    boost::asio::io_service::work m_TimeIOWork;
    boost::asio::deadline_timer m_CnTimer;
    boost::asio::deadline_timer m_WtTimer;
    boost::asio::deadline_timer m_RdTimer;
    
};

class TCPClientEx : public boost::noncopyable
{
public:
    TCPClientEx();
    ~TCPClientEx();

    boost::shared_ptr<TCPClient> CreateTCPClient(const char *pIPAddress, const char *pIPPort);

    void Run(const boost::uint32_t uiThreadNum, bool isWaitRunFinished = false);

    void Stop();

private:
    void RunIOService();

    //std::list<boost::shared_ptr<TCPClient> > m_TClientList;

    boost::asio::io_service m_IOService;    
    boost::asio::io_service::work m_IOWork;
    boost::thread_group m_RunThdGrp;
};

typedef boost::function<void(const boost::system::error_code&)>TimeOutCallback;

class TimeOutHandler : public boost::enable_shared_from_this<TimeOutHandler>, public boost::noncopyable
{
public:
    TimeOutHandler(TimeOutCallback cb, const boost::uint32_t uiTimeOutSec, const bool isLoop = true);
    TimeOutHandler(boost::asio::io_service &IOService, boost::asio::io_service::work &IOWork,
        TimeOutCallback cb, const boost::uint32_t uiTimeOutSec, const bool isLoop = true);
    ~TimeOutHandler();

    void Begin();

    void End();

    void SetTimeOutBase(const bool IsSecondBase);

    void Run();
        
    void Stop();

    void SetLoop(bool bIsLoop) { m_isLoop = bIsLoop; }

    void SetTimeOutCallBack(TimeOutCallback cb);

private:
    void RunTimeIOService();

    void HandleTimeOut(const boost::system::error_code& e);

private:
    boost::asio::io_service &m_TimeIOService;
    boost::asio::io_service::work &m_TimeIOWork;
    boost::shared_ptr<boost::asio::io_service> m_pTimeIOService;
    boost::shared_ptr<boost::asio::io_service::work> m_pTimeIOWork;
    
    boost::asio::deadline_timer m_Timer;
    
    TimeOutCallback m_cb;
    const boost::uint32_t m_uiTimeOutSec;
    boost::thread_group m_RunThdGrp;

    bool m_isLoop;

    bool m_IsSecondBase;

    boost::mutex m_LoopMutex;
    const unsigned int m_uiSeparateModel;
    bool m_IsEnded;
};


class TimeOutHandlerEx : public boost::noncopyable
{
public:
    TimeOutHandlerEx();
    ~TimeOutHandlerEx();

    boost::shared_ptr<TimeOutHandler> Create(TimeOutCallback cb, const boost::uint32_t uiTimeOutSec, const bool isLoop = true);

    void Run(const boost::uint32_t uiThreadNum, bool isWaitRunFinished = false);

    void Stop();

private:
    void RunIOService();

    boost::asio::io_service m_IOService;
    boost::asio::io_service::work m_IOWork;
    boost::thread_group m_RunThdGrp;
};


class Runner : public boost::noncopyable
{
public:
    Runner(const boost::uint32_t uiRunTdNum);
    ~Runner();

    void Post(RunCallFunc func);

    void PostSequence(RunCallFunc func, const std::string &strKey, const bool IsAsync = false);

    void Run(bool isWaitRunFinished = false, bool isSeqEnabled = false);

    void Stop();

private:
    void RunIOService();

    void RunIOServiceSeq();

    void PostSequenceInner(RunCallFunc func, const std::string &strKey);

    void SelfCall(boost::shared_ptr<boost::mutex> pRfcMutex, boost::shared_ptr<std::list<RunCallFunc> > pRfcList, const std::string &strKey);

private:
    boost::asio::io_service m_IOService;
    boost::asio::io_service::work m_IOWork;

    boost::thread_group m_RunThdGrp;

    boost::uint32_t m_uiRunTdNum;

    struct Rfc
    {
        boost::shared_ptr<boost::mutex> m_RfcMutex;
        boost::shared_ptr<std::list<RunCallFunc> > m_RfcList;
    };

    typedef std::unordered_map<std::string,  Rfc> TaskMap;
    TaskMap m_PendingTaskMap;
    boost::shared_mutex m_PendingMutex;

    boost::asio::io_service m_IOServiceSeq;
    boost::asio::io_service::work m_IOWorkSeq;

};

class RunnerEx
{
public:
    RunnerEx(const boost::uint32_t uiRunTdNum, const boost::uint32_t uiMaxQueueNum = 0);
    ~RunnerEx();

    void SetMaxQueueNum(const boost::uint32_t uiMaxQueueNum);

    inline bool Post(RunCallFunc func)
    {
        if ((0 < m_uiMaxQueueNum) && (m_uiMaxQueueNum <= m_uiCurrentQueueNum))
        {
            return false;
        }

        m_Runner.Post(boost::bind(&RunnerEx::SelfCall, this, func));
        ++m_uiCurrentQueueNum;
        return true;
    };

    inline void Run(bool isWaitRunFinished = false)
    {
        m_Runner.Run(isWaitRunFinished);
    };

    inline void Stop()
    {
        m_Runner.Stop();
    };

private:
    inline void SelfCall(RunCallFunc func)
    {
        func();
        --m_uiCurrentQueueNum;
    };

private:
    boost::uint32_t m_uiMaxQueueNum;
    boost::atomic_uint32_t m_uiCurrentQueueNum;
    Runner m_Runner;
};


class RunnerFree
{
public:
    RunnerFree(const boost::uint32_t uiRunTdNum);
    ~RunnerFree();

    bool Post(const RunCallFunc &func);

    void Run(bool isWaitRunFinished = false);

    void Stop();

private:
    void ConsumeFunc();

private:
    boost::lockfree::queue<RunCallFunc *> m_Queue;
    boost::thread_group m_RunThdGrp;
    boost::uint32_t m_uiRunTdNum;
    boost::atomic_bool m_NeedLoop;
};


/************************************************************************/
/* 提供通用计时处理，管理多个tick                                                  */
/************************************************************************/
class TickCountMgr : public boost::noncopyable
{
public:
    TickCountMgr();
    ~TickCountMgr();

    typedef boost::function<void(const std::string &)> TickCB;
    
    bool Add(const std::string &strKey, const boost::uint64_t uiMaxTick, TickCB tcb, 
        boost::shared_ptr<boost::atomic_uint64_t> &pTick, const bool IsReset = false, const bool IsAsync = true);

    bool Remove(const std::string &strKey, const bool IsAsync = true);
        
    bool Reset(const std::string &strKey, const bool IsAsync = true);

    bool Reset(boost::shared_ptr<boost::atomic_uint64_t> pTick);

    //bool Find(const std::string &strKey);

private:
    struct PropetyTick
    {
        bool m_IsReset; //超时之后是否重置tick值

        int m_uiStatus;
        boost::shared_ptr<boost::atomic_uint64_t> m_pTick;
        boost::uint64_t m_uiMaxTick;
        TickCB m_Tcb;

        static const int NORMAL = 0;
        static const int TIMEOUT = 1;
    };

    typedef std::unordered_map<std::string, PropetyTick> TickMap;

    TickMap m_TickMap;
    boost::mutex m_TickMutex;

    TimeOutHandler m_TMHandler;

    Runner m_Runner;

private:

    void TMHandler(const boost::system::error_code &ec);

    void TMHandlerInner(const boost::system::error_code &ec);

    bool AddInner(const std::string &strKey, const boost::uint64_t uiMaxTick, TickCB tcb,
        boost::shared_ptr<boost::atomic_uint64_t> &pTick, const bool IsReset = false);

    bool RemoveInner(const std::string &strKey);

    bool ResetInner(const std::string &strKey);

};


#endif

