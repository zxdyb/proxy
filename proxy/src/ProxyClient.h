#ifndef _PROXY_CLIENT_
#define _PROXY_CLIENT_

#include "NetComm.h"
#include <list>
#include <string>
#include <unordered_map>

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