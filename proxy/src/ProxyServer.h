#ifndef PROXY_SERVER
#define PROXY_SERVER

#ifdef _WIN32
#include "stdafx.h"
#include "../Storage/NetComm.h"
#else
#include "NetComm.h"
#endif

#include <list>
#include <string>
//#include <map>
#include <unordered_map>
//#include <boost/lockfree/spsc_queue.hpp>
#define BUFFER_SIZE (64 * 1024)
#include <boost/weak_ptr.hpp>


typedef std::unordered_map<std::string, boost::shared_ptr<std::list<std::string> > > PendingMap;

class ProxyHub;

class ProxySession : public boost::enable_shared_from_this<ProxySession>
{
public:
    ProxySession(ProxyHub &proxyhub, boost::shared_ptr<TCPSessionOfServer> pSession);
    ~ProxySession();

    void ReadCB(boost::shared_ptr<TCPSessionOfServer> pSession,
        const boost::system::error_code &ec, std::size_t bytes_transferred, void *pValue);

    void WriteCB(boost::shared_ptr<TCPSessionOfServer> pSession,
        const boost::system::error_code &ec, std::size_t bytes_transferred, void *pValue);
    
    void SyncWrite(char *pInputBuffer, const boost::uint32_t uiBufferSize);

    void SetSeqID(const boost::uint64_t uiSeq);

    boost::uint64_t GetSeqID();
    
    void SetID(const std::string &strID);

    inline void SetZeroID()
    {
        m_strZeroID = "0";
    };

    const std::string& GetID();

    void BindRead();
    
    inline boost::uint32_t GetStatus()
    {
        boost::shared_lock<boost::shared_mutex> lock(m_StatusMutex);
        return m_Status;
    };

    inline void SetStatus(const boost::uint32_t uiStatus)
    {
        boost::unique_lock<boost::shared_mutex> lock(m_StatusMutex);
        m_Status = uiStatus;
    };

    
    static const boost::uint32_t RUN = 1;
    static const boost::uint32_t STOP = 0;
    static const boost::uint32_t SUSPEND = 2;

private:
    bool ParseProtocol(char *pBuffer, const boost::uint32_t uiSize, std::list<std::string> *strProtoList,
        boost::uint32_t &uiFlag, char *&pBufferNewPos, std::list<std::string> *pDstIDList);
    
    ProxyHub &m_ProxyHub;

    std::string m_strID;
    std::string m_strZeroID;

    boost::uint32_t m_Status;
    boost::shared_mutex m_StatusMutex;

    boost::function<void()> m_Func;
    
    boost::mutex m_SendingMutex;

    boost::weak_ptr<TCPSessionOfServer> m_pTCPSession;

    boost::uint64_t m_SeqNum;
    std::string m_strSeqNum;
            
};

class ProxyHub
{
public:
    ProxyHub(const unsigned short uiPort);
    ~ProxyHub();

    void AcceptCB(boost::shared_ptr<TCPSessionOfServer> pSession, const boost::system::error_code &ec);

    void Run(const boost::uint32_t uiThreadNum);

    void Stop();


    void ProcessProtocol(boost::shared_ptr<std::list<std::string> > pProtoList,
        boost::shared_ptr<std::list<std::string> > pDstIDList,
        boost::shared_ptr<ProxySession> pSrcSession);

    void RemoveExangeMap(const std::string strID, const boost::uint64_t uiSeq);

    void AddExangeMap(const std::string &strID, boost::shared_ptr<ProxySession> pSession);

    void AddExangeMapZero(boost::shared_ptr<ProxySession> pSession, const std::string &strMsg);

private:

    TCPServer m_TSvr;

    typedef std::unordered_map<std::string, boost::shared_ptr<ProxySession> > ExangeMap; //key is bussness id,
    ExangeMap m_ExangeMap;
    
    boost::shared_mutex m_ExangeMutex;

    boost::atomic_bool m_NeedAuth;

    boost::atomic_uint64_t m_SeqNum;

private:

    bool IsBussnessAuth(char *pPos, boost::uint32_t uiReadPos);

};

class ProxyClient
{
public:
    ProxyClient(const std::string &strSrcIDBegin, const std::string &strDstIDBegin, const bool IsOnlyOneDst, const boost::uint32_t uiLinkNum, const boost::uint32_t uiMsgBodyLen,
        const char *pIPAddress, const char *pIPPort);
    ~ProxyClient();

    void Run(const boost::uint32_t uiThreadNum, const boost::uint32_t uiModule);

    void Stop();

    void ConnectedCB(boost::shared_ptr<TCPClient> pClient, const boost::system::error_code &ec, 
        boost::shared_ptr<char> pPendingBuffer, const boost::uint32_t uiTotalLen);

    void ReadCB(boost::shared_ptr<TCPClient> pClient, const boost::system::error_code &ec, std::size_t bytes_transferred, void *pValue, 
        boost::shared_ptr<char> pPendingBuffer, const boost::uint32_t uiTotalLen);

    void WriteCB(boost::shared_ptr<TCPClient> pClient, const boost::system::error_code &ec, std::size_t bytes_transferred, void *pValue, 
        boost::shared_ptr<char> pPendingBuffer, const boost::uint32_t uiTotalLen);

    static const boost::uint32_t SEND = 1;
    static const boost::uint32_t RECEIVE = 2;

private:

    char *GeneratePendingMsg(const std::string &strSrcID, const std::string &strDstID, const boost::uint32_t uiMsgBodyLen, boost::uint32_t &uiTotalLen);

private:

    std::string m_strSrcIDBegin;
    std::string m_strDstIDBegin;

    TCPClientEx m_TCPClientEx;
    boost::uint32_t m_uiLinkNum;
    boost::uint32_t m_uiMsgBodyLen;

    boost::uint32_t m_RunModule;
    
    

};

#endif