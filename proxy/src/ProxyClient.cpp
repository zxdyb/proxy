#include "ProxyClient.h"

ProxyClient::ProxyClient(const std::string &strSrcIDBegin, const std::string &strDstIDBegin, const bool IsOnlyOneDst,
    const boost::uint32_t uiLinkNum, const boost::uint32_t uiMsgBodyLen, const char *pIPAddress, const char *pIPPort) :
    m_strSrcIDBegin(strSrcIDBegin), m_strDstIDBegin(strDstIDBegin),
    m_uiLinkNum(uiLinkNum), m_uiMsgBodyLen(uiMsgBodyLen), m_RunModule(0)
{
    std::string strSrcID(strSrcIDBegin);
    std::string strDstID(strDstIDBegin);
    for (boost::uint32_t i = 1; i <= uiLinkNum; ++i)
    {
        char cTmp[32] = { 0 };
        boost::uint32_t uiTotalLen = 0;
        boost::shared_ptr<char> pPendingBuffer(GeneratePendingMsg(strSrcID, strDstID, uiMsgBodyLen, uiTotalLen));
        boost::shared_ptr<TCPClient> pTCPClient = m_TCPClientEx.CreateTCPClient(pIPAddress, pIPPort);
        pTCPClient->SetCallBack
            (
            boost::bind(&ProxyClient::ConnectedCB, this, pTCPClient, _1, pPendingBuffer, uiTotalLen), //NULL, NULL
            boost::bind(&ProxyClient::ReadCB, this, pTCPClient, _1, _2, _3, pPendingBuffer, uiTotalLen),
            boost::bind(&ProxyClient::WriteCB, this, pTCPClient, _1, _2, _3, pPendingBuffer, uiTotalLen)
            );
        pTCPClient->AsyncConnect();

        boost::int32_t iSrcID = atoi(strSrcID.c_str());
        iSrcID++;
        //strSrcID = itoa(iSrcID, cTmp, 10);
#ifdef _WIN32
        _snprintf(cTmp, sizeof(cTmp), "%d", iSrcID);
#else
        snprintf(cTmp, sizeof(cTmp), "%d", iSrcID);
#endif
        strSrcID = cTmp;

        if (!IsOnlyOneDst)
        {
            memset(cTmp, 0, sizeof(cTmp));
            boost::int32_t iDstID = atoi(strDstID.c_str());
            iDstID++;
            //strDstID = itoa(iDstID, cTmp, 10);
#ifdef _WIN32
            _snprintf(cTmp, sizeof(cTmp), "%d", iDstID);
#else
            snprintf(cTmp, sizeof(cTmp), "%d", iDstID);
#endif
            strDstID = cTmp;
        }

    }

}

ProxyClient::~ProxyClient()
{
    printf("proxy client destroyed.\n");

}

void ProxyClient::Run(const boost::uint32_t uiThreadNum, const boost::uint32_t uiModule)
{
    m_RunModule = uiModule;
    m_TCPClientEx.Run(uiThreadNum, true);
}

void ProxyClient::Stop()
{
    m_TCPClientEx.Stop();
}

void ProxyClient::ConnectedCB(boost::shared_ptr<TCPClient> pClient, const boost::system::error_code &ec,
    boost::shared_ptr<char> pPendingBuffer, const boost::uint32_t uiTotalLen)
{
    printf("connect  cb.\n");
    if (ec)
    {
        printf("connnect failed, %s\n", ec.message().c_str());
        return;
    }


    boost::uint32_t uiTotalLen2 = 0;
    boost::shared_ptr<char> pBuffer(GeneratePendingMsg(m_strSrcIDBegin, m_strSrcIDBegin, 10, uiTotalLen2));

    const char *pHeartMsg = pBuffer.get();
    boost::uint32_t uiSizeOfWrited = 0;
    std::string strErrorMsg;
    if (!pClient->SyncWrite((char*)pHeartMsg, strlen(pHeartMsg), uiSizeOfWrited, strErrorMsg))
    {
        printf("send echo msg error, %s\n", strErrorMsg.c_str());
        return;
    }
    printf("send echo msg.\n");

    ///////
    //char cHeartMsg[64] = { 0 };
    //boost::uint32_t uiSizeOfReaded = 0;
    //if (!pClient->SyncRead(cHeartMsg, sizeof(cHeartMsg), uiSizeOfReaded, strErrorMsg))
    //{
    //    printf("receive heart msg error, %s\n", strErrorMsg.c_str());
    //    return;
    //}
    //printf("receive heart msg, %s\n", cHeartMsg);

    //pClient->SetCallBack
    //( 
    //boost::bind(&ProxyClient::ConnectedCB, this, pClient, _1, pPendingBuffer, uiTotalLen),
    //boost::bind(&ProxyClient::ReadCB, this, pClient, _1, _2, _3, pPendingBuffer, uiTotalLen),
    //boost::bind(&ProxyClient::WriteCB, this, pClient, _1, _2, _3, pPendingBuffer, uiTotalLen)
    //);

    if (m_RunModule & SEND)
    {
        pClient->AsyncWrite(pPendingBuffer.get(), uiTotalLen);
    }

    if (m_RunModule & RECEIVE)
    {
        char *pReadBuffer = new char[1024];
        pClient->AsyncRead(pReadBuffer, 1024, 0, (void*)pReadBuffer);
    }

}

void ProxyClient::ReadCB(boost::shared_ptr<TCPClient> pClient, const boost::system::error_code &ec, std::size_t bytes_transferred, void *pValue,
    boost::shared_ptr<char> pPendingBuffer, const boost::uint32_t uiTotalLen)
{
    if (ec)
    {
        printf("read cb error, %s.\n", ec.message().c_str());
        return;
    }

    char *pReadBuffer = (char *)pValue;
    pClient->AsyncRead(pReadBuffer, 1024, 0, (void*)pReadBuffer);

    //printf("read msg %s\n", pReadBuffer);

    /*char *pReadBuffer = (char *)pValue;
    delete[] pReadBuffer;
    pReadBuffer = NULL;*/
}

void ProxyClient::WriteCB(boost::shared_ptr<TCPClient> pClient, const boost::system::error_code &ec, std::size_t bytes_transferred, void *pValue,
    boost::shared_ptr<char> pPendingBuffer, const boost::uint32_t uiTotalLen)
{
    if (ec)
    {
        printf("write cb error, %s.\n", ec.message().c_str());
        return;
    }

    pClient->AsyncWrite(pPendingBuffer.get(), uiTotalLen);
}


char *ProxyClient::GeneratePendingMsg(const std::string &strSrcID, const std::string &strDstID, const boost::uint32_t uiMsgBodyLen, boost::uint32_t &uiTotalLen)
{
    //RG,Len,src,dst,type,body,crc
    //RG,40,storage_upload_11111111111111111,0,0,0,0
    boost::uint32_t uiMsgLen = strSrcID.size() + 1 + strDstID.size() + 1 + 1 + 1 + uiMsgBodyLen + 1 + 1;

    char cTmp[256] = { 0 };
#ifdef _WIN32
    _snprintf(cTmp, sizeof(cTmp), "%d", uiMsgLen);
#else
    snprintf(cTmp, sizeof(cTmp), "%d", uiMsgLen);
#endif
    std::string strMsgLen(cTmp);

    uiMsgLen = uiMsgLen + 3 + strMsgLen.size() + 1;

    char *pMsg = new char[uiMsgLen];
    memset(pMsg, 'y', uiMsgLen);

#ifdef _WIN32
    int iPos = _snprintf(pMsg, uiMsgLen, "RG,%s,%s,%s,%s,", strMsgLen.c_str(), strSrcID.c_str(), strDstID.c_str(), "0");
#else
    int iPos = snprintf(pMsg, uiMsgLen, "RG,%s,%s,%s,%s,", strMsgLen.c_str(), strSrcID.c_str(), strDstID.c_str(), "0");
#endif
    pMsg[iPos] = 'y';

    pMsg[uiMsgLen - 2] = ',';
    pMsg[uiMsgLen - 1] = '0';

    std::string strTmp;
    strTmp.assign(pMsg, uiMsgLen);

    //m_pPendingMsg->assign(pMsg, uiMsgLen);

    uiTotalLen = uiMsgLen;
    return pMsg;
}

