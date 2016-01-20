#include "NetComm.h"
#include <string>

using boost::asio::ip::tcp;


UDPClient::UDPClient(const char *pIPAddress, const boost::uint16_t uiPort)
    : m_address(boost::asio::ip::address::from_string(pIPAddress)), m_endpoint(m_address, uiPort), m_socket(m_ioservice), 
    m_InitSucceed(false)
{
    boost::system::error_code e;
    m_socket.open(boost::asio::ip::udp::v4(), e);

    if (e)
    {
        m_InitSucceed = false;
    }
    else
    {
        m_InitSucceed = true;
    }
}


UDPClient::~UDPClient(void)
{
    m_socket.close();
}

bool UDPClient::SendMessageSyn( const char *pMessage, const boost::uint32_t uiSize )
{
    if (!m_InitSucceed)
    {
        return false;
    }

    boost::system::error_code e;
    m_socket.send_to(boost::asio::buffer(pMessage, uiSize), m_endpoint, 0, e);

    if (e)
    {
        return false;
    }
    
    return true;

}

UDPServer::UDPServer( const boost::uint16_t uiPort) : 
m_socket(m_ioservice, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), uiPort))
{

}

UDPServer::~UDPServer()
{
    m_socket.close();
}

bool UDPServer::ReceiveMessageSyn( char *pMessage, const boost::uint32_t uiSize )
{
    boost::asio::ip::udp::endpoint remote_endpoint;
    boost::system::error_code error;

    // 接收一个字符，这样就得到了远程端点(remote_endpoint)
    m_socket.receive_from(boost::asio::buffer(pMessage, uiSize),
        remote_endpoint, 0, error);

    if (error) //&& error != boost::asio::error::message_size)
    {
        //throw boost::system::system_error(error);
        return false;
    }
    else
    {
        return true;
    }
}

TCPSessionOfServer::TCPSessionOfServer(boost::asio::io_service& io_service, 
	ServerReadOrWriteCallback RdCB, ServerReadOrWriteCallback WtCB) : m_Socket(io_service), m_RdCB(RdCB), m_WtCB(WtCB)
{

}

tcp::socket& TCPSessionOfServer::GetSocket()
{
	return m_Socket;
}


void TCPSessionOfServer::AsyncWrite(char *pInputBuffer, const boost::uint32_t uiBufferSize, //const boost::uint32_t uiWtTimeOutSec,
    void *pValue)
{
    boost::asio::async_write(m_Socket, boost::asio::buffer(pInputBuffer, uiBufferSize), 
        boost::bind(&TCPSessionOfServer::HandleWrite, shared_from_this(), //pTimer,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred, pValue));
}

void TCPSessionOfServer::AsyncRead(char *pOutputBuffer, const boost::uint32_t uiBufferSize, //const boost::uint32_t uiRdTimeOutSec,
    void *pValue, const boost::uint32_t uiNeedReadSize)
{
    if (0 == uiNeedReadSize)
    {
        m_Socket.async_read_some(boost::asio::buffer(pOutputBuffer, uiBufferSize),
            boost::bind(&TCPSessionOfServer::HandleRead, shared_from_this(), //pTimer,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred, pValue));
    }
    else
    {
        boost::asio::async_read(m_Socket, boost::asio::buffer(pOutputBuffer, uiNeedReadSize),
            boost::bind(&TCPSessionOfServer::HandleRead, shared_from_this(), //pTimer,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred, pValue));
    }
}



void TCPSessionOfServer::HandleRead(const boost::system::error_code& error, std::size_t bytes_transferred, void *pValue)
{
	if (NULL != m_RdCB)
    {
        m_RdCB(shared_from_this(), error, bytes_transferred, pValue);
    }

}

void TCPSessionOfServer::HandleWrite(const boost::system::error_code& error, std::size_t bytes_transferred, void *pValue)
{

    if (NULL != m_WtCB)
    {
        m_WtCB(shared_from_this(), error, bytes_transferred, pValue);
    }

}

void TCPSessionOfServer::SetCallBack(ServerReadOrWriteCallback cb, const bool IsReadCB)
{
    if (IsReadCB)
    {
        m_RdCB = cb;
    }
    else
    {
        m_WtCB = cb;
    }
}

TCPSessionOfServer::~TCPSessionOfServer()
{
    printf("Tcp session is destroy.\n");
}

bool TCPSessionOfServer::SyncWrite(char *pInputBuffer, const boost::uint32_t uiBufferSize, 
    boost::uint32_t &uiSizeofWrited, std::string &strErrMsg)
{
    boost::system::error_code ec;

    uiSizeofWrited = boost::asio::write(m_Socket, boost::asio::buffer(pInputBuffer, uiBufferSize), ec);

    if (ec)
    {
        strErrMsg = ec.message();
        return false;
    }

    return true;
}

TCPServer::TCPServer(unsigned short port) : m_Acceptor(m_IOServiceAcceptor, tcp::endpoint(tcp::v4(), port)), 
m_IOWork(m_IOService), m_IOWorkAcceptor(m_IOServiceAcceptor), m_AcceptedCallback(NULL), m_SessionNum(0)
{
	//StartAccept();
}

void TCPServer::StartAccept(ServerReadOrWriteCallback RdBck, ServerReadOrWriteCallback WtBck)
{
    ++m_SessionNum;

    boost::asio::io_service *pNewIOService = NULL;
    if (1 == m_SessionNum)
    {
        pNewIOService = SessionIOService().m_IOService;
        new boost::thread(boost::bind(&TCPServer::RunCommonIOService, this, pNewIOService));
    }

    boost::shared_ptr<TCPSessionOfServer> pSession(
        new TCPSessionOfServer(*(NULL == pNewIOService ? &m_IOService : pNewIOService),
        RdBck, WtBck));

	m_Acceptor.async_accept(pSession->GetSocket(),
		boost::bind(&TCPServer::HandleAccept, this, pSession,
		boost::asio::placeholders::error, RdBck, WtBck));
}

void TCPServer::HandleAccept(boost::shared_ptr<TCPSessionOfServer> pSession, const boost::system::error_code& error,
	ServerReadOrWriteCallback RdBck, ServerReadOrWriteCallback WtBck)
{
	
	if (error)
    {
        std::cout << "HandleAccept is error, msg is " << error.message().c_str() << std::endl;
		return;
    }

	if (NULL != m_AcceptedCallback)
	{
		m_AcceptedCallback(pSession, error);
	}

	//pSession->Start();///////////////////////////////

    StartAccept(RdBck, WtBck);
}

void TCPServer::Run(const boost::uint32_t uiThreadNum, AcceptedCallback ApBck, ServerReadOrWriteCallback RdBck, ServerReadOrWriteCallback WtBck,
	const bool isWaitRunFinished)
{
	m_AcceptedCallback = ApBck;

	StartAccept(RdBck, WtBck);

    new boost::thread(boost::bind(&TCPServer::RunCommonIOService, this, &m_IOServiceAcceptor));

	for (boost::uint32_t i = 0; i < uiThreadNum; ++i)
	{
		m_RunThdGrp.add_thread(new boost::thread(boost::bind(&TCPServer::RunIOService, this)));
	}


	//m_RunThdGrp.add_thread(new boost::thread(boost::bind(&TCPClient::RunTimeIOService, this)));

	if (isWaitRunFinished)
	{
		m_RunThdGrp.join_all();
	}
}

void TCPServer::RunIOService()
{
    boost::system::error_code error;
    m_IOService.run(error);
}

void TCPServer::RunCommonIOService(boost::asio::io_service *pIoSvr)
{
    boost::system::error_code error;
    pIoSvr->run(error);
}

/////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////

TCPSessionOfClient::TCPSessionOfClient(boost::asio::io_service &TMIOService,
    boost::asio::io_service& io_service, 
    ReadOrWriteCallback RdBck, ReadOrWriteCallback WtBck,
    const boost::uint32_t uiRdTimeOutSec, const boost::uint32_t uiWtTimeOutSec) : 
    m_TMIOService(TMIOService),
    m_Socket(io_service), m_RdBck(RdBck), m_WtBck(WtBck),
    m_uiRdTimeOutSec(uiRdTimeOutSec), m_uiWtTimeOutSec(uiWtTimeOutSec)

{
}

boost::asio::ip::tcp::socket& TCPSessionOfClient::GetSocket()
{
    return m_Socket;
}

void TCPSessionOfClient::HandleWrite(boost::shared_ptr<boost::asio::deadline_timer> pTimer, 
    const boost::system::error_code& ec, std::size_t bytes_transfered,
    void *pValue)
{
    {
        if (NULL != pTimer.get())
        {
            boost::system::error_code et;
            pTimer->cancel(et);
        }        
    }

    if (NULL != m_WtBck)
    {
        m_WtBck(ec, bytes_transfered, pValue);
    }
    
}

void TCPSessionOfClient::HandleRead(boost::shared_ptr<boost::asio::deadline_timer> pTimer, 
    const boost::system::error_code& ec, std::size_t bytes_transfered, void *pValue)
{
    {
        if (NULL != pTimer.get())
        {
            boost::system::error_code et;
            pTimer->cancel(et);
        } 
    }

    if (NULL != m_RdBck)
    {
        m_RdBck(ec, bytes_transfered, pValue);
    }
    
    //AsyncRead(pOutputBuffer, uiBufferSize, uiRdTimeOutSec, pValue);
}

void TCPSessionOfClient::AsyncRead(char *pOutputBuffer, const boost::uint32_t uiBufferSize, 
                                                        const boost::uint32_t uiRdTimeOutSec,
                                                        void *pValue, const boost::uint32_t uiNeedReadSize)
{
        
    boost::uint32_t uiTimeOutSec = 0;

    uiTimeOutSec = (0 != uiRdTimeOutSec) ? uiRdTimeOutSec : m_uiRdTimeOutSec;
    boost::shared_ptr<boost::asio::deadline_timer> pTimer;
    if (0 != uiTimeOutSec)
    {
        boost::system::error_code ecn;
        pTimer.reset(new boost::asio::deadline_timer(m_TMIOService));
        pTimer->expires_from_now(boost::posix_time::seconds(uiTimeOutSec), ecn);  
        pTimer->async_wait(boost::bind(&TCPSessionOfClient::HandleRdTimeOut, shared_from_this(), 
            boost::asio::placeholders::error));
    }

    if (0 == uiNeedReadSize)
    {
        m_Socket.async_read_some(boost::asio::buffer(pOutputBuffer, uiBufferSize),
                boost::bind(&TCPSessionOfClient::HandleRead, shared_from_this(), pTimer,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred, pValue));
    }
    else
    {
        boost::asio::async_read(m_Socket, boost::asio::buffer(pOutputBuffer, uiNeedReadSize),
            boost::bind(&TCPSessionOfClient::HandleRead, shared_from_this(), pTimer,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred, pValue));
    }    
    
}

void TCPSessionOfClient::AsyncWrite(char *pInputBuffer, const boost::uint32_t uiBufferSize, 
                                                        const boost::uint32_t uiWtTimeOutSec,
                                                        void *pValue)
{
    boost::uint32_t uiTimeOutSec = 0;

    uiTimeOutSec = (0 != uiWtTimeOutSec) ? uiWtTimeOutSec : m_uiWtTimeOutSec;
    boost::shared_ptr<boost::asio::deadline_timer> pTimer;
    if (0 != uiTimeOutSec)
    {
        boost::system::error_code ecn;
        pTimer.reset(new boost::asio::deadline_timer(m_TMIOService));
        pTimer->expires_from_now(boost::posix_time::seconds(uiTimeOutSec), ecn);  
        pTimer->async_wait(boost::bind(&TCPSessionOfClient::HandleWtTimeOut, shared_from_this(), 
            boost::asio::placeholders::error));
    }

    boost::asio::async_write(m_Socket, boost::asio::buffer(pInputBuffer, uiBufferSize), 
        boost::bind(&TCPSessionOfClient::HandleWrite, shared_from_this(), pTimer,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred, pValue));

}

void TCPSessionOfClient::HandleWtTimeOut(const boost::system::error_code& e)
{
    SocketCancel(e);
}

void TCPSessionOfClient::HandleRdTimeOut(const boost::system::error_code& e)
{
    SocketCancel(e);
}

void TCPSessionOfClient::SocketCancel(const boost::system::error_code& e)
{
    
    if (e)
    {
        return; //Timer canceled.
    }

    boost::system::error_code ec;
    m_Socket.cancel(ec);
    if (ec)
    {
        //log to file
    }
}

bool TCPSessionOfClient::SyncWrite(char *pInputBuffer, const boost::uint32_t uiBufferSize, boost::uint32_t &uiSizeofWrited, std::string &strErrMsg)
{
    boost::system::error_code ec;

    uiSizeofWrited = boost::asio::write(m_Socket, boost::asio::buffer(pInputBuffer, uiBufferSize), ec);

    if (ec)
    {
        strErrMsg = ec.message();
        return false;
    }

    return true;
}

bool TCPSessionOfClient::SyncRead(char *pOutputBuffer, const boost::uint32_t uiBufferSize, boost::uint32_t &uiSizeofReaded, std::string &strErrMsg)
{
    boost::system::error_code ec;

    uiSizeofReaded = boost::asio::read(m_Socket, boost::asio::buffer(pOutputBuffer, uiBufferSize), ec);

    if (ec)
    {
        strErrMsg = ec.message();
        return false;
    }

    return true;

}

TCPClient::TCPClient(const char *pIPAddress, const char *pIPPort, ConnectedCallback CnBck, ReadOrWriteCallback RdBck,
    ReadOrWriteCallback WtBck,
    const boost::uint32_t uiCnTimeOutSec, 
    const boost::uint32_t uiRdTimeOutSec,
    const boost::uint32_t uiWtTimeOutSec) : m_strIPAddress(pIPAddress), m_strIPPort(pIPPort),
                                                                    m_CnBck(CnBck), m_IOService(*new boost::asio::io_service), 
                                                                    m_IOWork(*new boost::asio::io_service::work(m_IOService)),
                                                                    m_uiCnTimeOutSec(uiCnTimeOutSec),
                                                                    m_uiRdTimeOutSec(uiRdTimeOutSec),
                                                                    m_uiWtTimeOutSec(uiWtTimeOutSec),
                                                                    m_TimeIOWork(m_TimeIOService),
                                                                    m_CnTimer(m_TimeIOService),
                                                                    m_WtTimer(m_TimeIOService),
                                                                    m_RdTimer(m_TimeIOService)
{

    m_Session.reset(new TCPSessionOfClient(m_TimeIOService, m_IOService, RdBck, WtBck, uiRdTimeOutSec, uiWtTimeOutSec));
    AsyncConnect(uiCnTimeOutSec);

}

TCPClient::TCPClient(const char *pIPAddress, const char *pIPPort) : m_strIPAddress(pIPAddress), m_strIPPort(pIPPort),
                                                        m_CnBck(NULL),
                                                        m_IOService(*new boost::asio::io_service),
                                                        m_IOWork(*new boost::asio::io_service::work(m_IOService)),
                                                        m_uiCnTimeOutSec(0),
                                                        m_uiRdTimeOutSec(0),
                                                        m_uiWtTimeOutSec(0),
                                                        m_TimeIOWork(m_TimeIOService),
                                                        m_CnTimer(m_TimeIOService),
                                                        m_WtTimer(m_TimeIOService),
                                                        m_RdTimer(m_TimeIOService)
{

}

TCPClient::TCPClient(const char *pIPAddress, const char *pIPPort, 
                                                        boost::asio::io_service &IOService, boost::asio::io_service::work &IOWork) :
                                                        m_strIPAddress(pIPAddress), m_strIPPort(pIPPort),
                                                        m_CnBck(NULL),
                                                        m_IOService(IOService),
                                                        m_IOWork(IOWork),
                                                        m_uiCnTimeOutSec(0),
                                                        m_uiRdTimeOutSec(0),
                                                        m_uiWtTimeOutSec(0),
                                                        m_TimeIOWork(m_TimeIOService),
                                                        m_CnTimer(m_TimeIOService),
                                                        m_WtTimer(m_TimeIOService),
                                                        m_RdTimer(m_TimeIOService)
{

}

void TCPClient::HandleCnTimeOut(const boost::system::error_code& e)
{
    SocketCancel(e);
}



void TCPClient::SocketCancel(const boost::system::error_code& e)
{
    if (e)
    {
        return; //Timer canceled.
    }

    boost::system::error_code ec;
    m_Session->GetSocket().cancel(ec);
    if (ec)
    {
        //log to file
    }
}

void TCPClient::Stop()
{
    m_TimeIOService.stop();

    boost::system::error_code ec;
    m_Session->GetSocket().close(ec);
    if (ec)
    {
        
    }
    m_IOService.stop();    

    m_RunThdGrp.join_all();

    delete &m_IOWork;
    delete &m_IOService;
}

void TCPClient::RunIOService()
{
    boost::system::error_code error;   
    m_IOService.run(error);
    if (error)
    {
        
    }
}

void TCPClient::RunTimeIOService()
{
    boost::system::error_code error;   
    m_TimeIOService.run(error);
    if (error)
    {
        
    }
}

void TCPClient::Run(const boost::uint32_t uiThreadNum, bool isWaitRunFinished)
{
    for (boost::uint32_t i = 0; i < uiThreadNum; ++i)
    {
        m_RunThdGrp.add_thread(new boost::thread(boost::bind(&TCPClient::RunIOService, this)));
    }

    m_RunThdGrp.add_thread(new boost::thread(boost::bind(&TCPClient::RunTimeIOService, this)));

    if (isWaitRunFinished)
    {
        m_RunThdGrp.join_all();
    }
}

void TCPClient::AsyncConnect(const boost::uint32_t uiCnTimeOutSec /*= 0*/)
{
    if (0 == m_Session.get())
    {
        return;
    }

    boost::asio::ip::tcp::resolver resolver(m_IOService);
    boost::asio::ip::tcp::resolver::iterator endpoint = resolver.resolve(tcp::resolver::query(m_strIPAddress.c_str(), 
        m_strIPPort.c_str()));

    boost::asio::async_connect(m_Session->GetSocket(), endpoint, 
        boost::bind(&TCPClient::HandleConnect, this, m_Session, _1));

    boost::uint32_t uiTimeOutSec = 0;

    uiTimeOutSec = (0 != uiCnTimeOutSec) ? uiCnTimeOutSec : m_uiCnTimeOutSec;

    if (0 != uiTimeOutSec)
    {
        boost::system::error_code ecn;
        m_CnTimer.expires_from_now(boost::posix_time::seconds(uiTimeOutSec), ecn);  
        m_CnTimer.async_wait(boost::bind(&TCPClient::HandleCnTimeOut, this, boost::asio::placeholders::error));

    }
}


void TCPClient::Run(const boost::uint32_t uiThreadNum, ConnectedCallback CnBck, ReadOrWriteCallback RdBck,
    ReadOrWriteCallback WtBck, bool isWaitRunFinished,
    const boost::uint32_t uiCnTimeOutSec, 
    const boost::uint32_t uiRdTimeOutSec,
    const boost::uint32_t uiWtTimeOutSec)
{
    m_CnBck = CnBck;
    m_uiCnTimeOutSec = uiCnTimeOutSec;
    m_uiRdTimeOutSec = uiRdTimeOutSec;
    m_uiWtTimeOutSec = uiWtTimeOutSec;

    m_Session.reset(new TCPSessionOfClient(m_TimeIOService, m_IOService, RdBck, WtBck, uiRdTimeOutSec, uiWtTimeOutSec));

    AsyncConnect(uiCnTimeOutSec);
    
    Run(uiThreadNum, isWaitRunFinished);
}

void TCPClient::HandleConnect(boost::shared_ptr<TCPSessionOfClient> session_ptr, const boost::system::error_code& ec)
{
    (void)session_ptr;
    
    {
        boost::system::error_code et;
        m_CnTimer.cancel(et);
    }

    if (NULL != m_CnBck)
    {
        m_CnBck(ec);
    }
    
}

void TCPClient::AsyncRead(char *pOutputBuffer, const boost::uint32_t uiBufferSize, 
    const boost::uint32_t uiRdTimeOutSec, void *pValue, const boost::uint32_t uiNeedReadSize)
{
    m_Session->AsyncRead(pOutputBuffer, uiBufferSize, uiRdTimeOutSec, pValue, uiNeedReadSize);
}

void TCPClient::AsyncWrite(char *pInputBuffer, const boost::uint32_t uiBufferSize, 
    const boost::uint32_t uiWtTimeOutSec, void *pValue)
{
    m_Session->AsyncWrite(pInputBuffer, uiBufferSize, uiWtTimeOutSec, pValue);
}

void TCPClient::Post(RunCallFunc func)
{
    m_IOService.post(func);
}

bool TCPClient::SyncWrite(char *pInputBuffer, const boost::uint32_t uiBufferSize, boost::uint32_t &uiSizeofWrited,
    std::string &strErrMsg)
{
    return m_Session->SyncWrite(pInputBuffer, uiBufferSize, uiSizeofWrited, strErrMsg);
}

bool TCPClient::SyncRead(char *pOutputBuffer, const boost::uint32_t uiBufferSize, boost::uint32_t &uiSizeofReaded, std::string &strErrMsg)
{
    return m_Session->SyncRead(pOutputBuffer, uiBufferSize, uiSizeofReaded, strErrMsg);
}

void TCPClient::SetCallBack(ConnectedCallback CnBck, ReadOrWriteCallback RdBck, ReadOrWriteCallback WtBck, 
    const boost::uint32_t uiCnTimeOutSec /*= 0*/, 
    const boost::uint32_t uiRdTimeOutSec /*= 0*/, 
    const boost::uint32_t uiWtTimeOutSec /*= 0*/)
{
    m_CnBck = CnBck;
    m_uiCnTimeOutSec = uiCnTimeOutSec;
    m_uiRdTimeOutSec = uiRdTimeOutSec;
    m_uiWtTimeOutSec = uiWtTimeOutSec;

    m_Session.reset(new TCPSessionOfClient(m_TimeIOService, m_IOService, RdBck, WtBck, uiRdTimeOutSec, 
        uiWtTimeOutSec));

}

//void TCPClient::Close()
//{
//    m_TimeIOService.stop();
//
//    boost::system::error_code ec;
//    m_Session->GetSocket().cancel(ec);
//    if (ec)
//    {
//
//    }
//}

TimeOutHandler::TimeOutHandler(TimeOutCallback cb, const boost::uint32_t uiTimeOutSec, const bool isLoop) : 
m_TimeIOWork(m_TimeIOService), m_Timer(m_TimeIOService), m_cb(cb), m_uiTimeOutSec(uiTimeOutSec), m_isLoop(isLoop), m_IsSecondBase(true)
{
    
}

TimeOutHandler::~TimeOutHandler()
{

}

void TimeOutHandler::Run()
{
    if (0 == m_uiTimeOutSec)
    {
        return;
    }

    m_RunThdGrp.add_thread(new boost::thread(boost::bind(&TimeOutHandler::RunTimeIOService, this)));

    boost::system::error_code ecn;
    if (m_IsSecondBase)
    {
        m_Timer.expires_from_now(boost::posix_time::seconds(m_uiTimeOutSec), ecn);
    }
    else
    {
        m_Timer.expires_from_now(boost::posix_time::milliseconds(m_uiTimeOutSec), ecn);
    }

    m_Timer.async_wait(boost::bind(&TimeOutHandler::HandleTimeOut, this, boost::asio::placeholders::error));

}

void TimeOutHandler::RunTimeIOService()
{
    boost::system::error_code error;   
    m_TimeIOService.run(error);
    if (error)
    {

    }
}

void TimeOutHandler::Stop()
{
    if (0 == m_uiTimeOutSec)
    {
        return;
    }

    m_TimeIOService.stop();

    m_RunThdGrp.join_all();
}

void TimeOutHandler::HandleTimeOut(const boost::system::error_code& e)
{
    if (NULL != m_cb)
    {
        m_cb(e);
    }

    if ((0 == m_uiTimeOutSec) || e || !m_isLoop)
    {
        return;
    }

    boost::system::error_code ecn;
    if (m_IsSecondBase)
    {
        m_Timer.expires_from_now(boost::posix_time::seconds(m_uiTimeOutSec), ecn);
    }
    else
    {
        m_Timer.expires_from_now(boost::posix_time::milliseconds(m_uiTimeOutSec), ecn);
    }
    
    m_Timer.async_wait(boost::bind(&TimeOutHandler::HandleTimeOut, this, boost::asio::placeholders::error));
}

void TimeOutHandler::SetTimeOutBase(const bool IsSecondBase)
{
    m_IsSecondBase = IsSecondBase;
}

Runner::Runner(const boost::uint32_t uiRunTdNum) :
m_IOWork(m_IOService), m_uiRunTdNum(uiRunTdNum)
{

}

Runner::~Runner()
{

}

void Runner::Run(bool isWaitRunFinished)
{
    if (0 == m_uiRunTdNum)
    {
        return;
    }

    for (boost::uint32_t i = 0; i < m_uiRunTdNum; ++i)
    {
        m_RunThdGrp.add_thread(new boost::thread(boost::bind(&Runner::RunIOService, this)));
    }

    if (isWaitRunFinished)
    {
        m_RunThdGrp.join_all();
    }

}

void Runner::RunIOService()
{
    boost::system::error_code error;
    m_IOService.run(error);
    if (error)
    {

    }
}

void Runner::Stop()
{
    if (0 == m_uiRunTdNum)
    {
        return;
    }

    m_IOService.stop();

    m_RunThdGrp.join_all();
}

void Runner::Post(RunCallFunc func)
{
    m_IOService.post(func);
}




TCPClientEx::TCPClientEx() : m_IOWork(m_IOService)
{

}

TCPClientEx::~TCPClientEx()
{

}

void TCPClientEx::Run(const boost::uint32_t uiThreadNum, bool isWaitRunFinished /*= false*/)
{
    for (boost::uint32_t i = 0; i < uiThreadNum; ++i)
    {
        m_RunThdGrp.add_thread(new boost::thread(boost::bind(&TCPClientEx::RunIOService, this)));
    }

    if (isWaitRunFinished)
    {
        m_RunThdGrp.join_all();
    }
}

void TCPClientEx::Stop()
{
    m_IOService.stop();

    m_RunThdGrp.join_all();
}

boost::shared_ptr<TCPClient> TCPClientEx::CreateTCPClient(const char *pIPAddress, const char *pIPPort)
{
    return boost::shared_ptr<TCPClient>(new TCPClient(pIPAddress, pIPPort, m_IOService, m_IOWork));
}

void TCPClientEx::RunIOService()
{
    boost::system::error_code error;
    m_IOService.run(error);
    if (error)
    {

    }
}

RunnerEx::RunnerEx(const boost::uint32_t uiRunTdNum, const boost::uint32_t uiMaxQueueNum /*= 0*/)
    : m_uiMaxQueueNum(uiMaxQueueNum), m_uiCurrentQueueNum(0), m_Runner(uiRunTdNum)
{

}

RunnerEx::~RunnerEx()
{

}

void RunnerEx::SetMaxQueueNum(const boost::uint32_t uiMaxQueueNum)
{
    m_uiMaxQueueNum = uiMaxQueueNum;
}

RunnerFree::RunnerFree(const boost::uint32_t uiRunTdNum) : m_Queue(128),
m_uiRunTdNum(uiRunTdNum), //
m_NeedLoop(true)
{

}

RunnerFree::~RunnerFree()
{

}

bool RunnerFree::Post(const RunCallFunc &func)
{
    if (!m_NeedLoop)
    {
        return false;
    }

    RunCallFunc *pFc = new RunCallFunc;
    *pFc = func;

    return m_Queue.push(pFc);
}

void RunnerFree::Run(bool isWaitRunFinished /*= false*/)
{
    if (0 == m_uiRunTdNum)
    {
        return;
    }

    for (boost::uint32_t i = 0; i < m_uiRunTdNum; ++i)
    {
        m_RunThdGrp.add_thread(new boost::thread(boost::bind(&RunnerFree::ConsumeFunc, this)));
    }

    if (isWaitRunFinished)
    {
        m_RunThdGrp.join_all();
    }
}

void RunnerFree::Stop()
{
    if (0 >= m_uiRunTdNum)
    {
        return;
    }

    m_NeedLoop = false;
    
    m_RunThdGrp.join_all();
}

void RunnerFree::ConsumeFunc()
{
    while (m_NeedLoop)
    {
        RunCallFunc *pFc;
        while (m_Queue.pop(pFc))
        {
            (*pFc)();

            delete pFc;
            pFc = NULL;
        }
    }
}
