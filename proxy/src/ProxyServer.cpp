#include "ProxyServer.h"
#include "LogRLD.h"
#include "json/json.h"
#include "boost/bind.hpp"
#include "boost/lexical_cast.hpp"
#include "CommonUtility.h"
#include "libcacheclient.h"

#ifdef WIN32
#ifndef snprintf
#define snprintf _snprintf
#endif
#endif

ProxyHub::ProxyHub(const unsigned short uiPort) : m_TSvr(uiPort), m_NeedAuth(true), m_SeqNum(0), m_SrcIDReplaceByIncSeq(false), m_CreateSIDOnConnected(false),
m_uiAsyncReadTimeOut(10), m_blAuthEnable(false), m_iAuthSrcPort(11211), m_pMemCl(NULL), m_AuthRunner(1)
{
    m_AuthRunner.Run();
}

ProxyHub::~ProxyHub()
{

}

void ProxyHub::AcceptCB(boost::shared_ptr<TCPSessionOfServer> pSession, const boost::system::error_code &ec)
{
    if (ec)
    {
        LOG_ERROR_RLD("Accept error: " << ec.message());
        return;
    }

    const std::string &strAddress = pSession->GetSocket().remote_endpoint().address().to_string();

    boost::shared_ptr<ProxySession> pProxySession(new ProxySession( *this, pSession));

    pProxySession->SetSrcIDReplaceByIncSeq(m_SrcIDReplaceByIncSeq);
    //if (m_SrcIDReplaceByIncSeq) //ȷ���������˵��������кţ����ܺ����ҵ����õ������Դ
    {        
        pProxySession->SetSeqID(++m_SeqNum);
    }
    pProxySession->SetCreateSIDOnConnected(m_CreateSIDOnConnected);

    pProxySession->SetAsyncReadTimeOut(m_uiAsyncReadTimeOut);

    LOG_INFO_RLD("Accept connection and remote address is " << strAddress.c_str() << " and seq id is " << pProxySession->GetSeqID());
    
    pSession->SetCallBack(boost::bind(&ProxySession::ReadCB, pProxySession, _1, _2, _3, _4), true);
    //pSession->SetCallBack(boost::bind(&ProxySession::WriteCB, pProxySession, _1, _2, _3, _4), false);

    pSession->SetTimeoutCB(boost::bind(&ProxySession::ReadTimeoutCB, pProxySession, _1), true);

    char *pBuffer = new char[BUFFER_SIZE];
    memset(pBuffer, 0, BUFFER_SIZE);

    pSession->AsyncRead(pBuffer + sizeof(boost::uint32_t), BUFFER_SIZE - sizeof(boost::uint32_t), m_uiAsyncReadTimeOut, (void *)pBuffer);


}

void ProxyHub::Run(const boost::uint32_t uiThreadNum)
{    
    LOG_INFO_RLD("Proxy begin running and thread number is " << uiThreadNum);

    m_TSvr.Run(uiThreadNum, boost::bind(&ProxyHub::AcceptCB, this, _1, _2), NULL, NULL, true);

}

void ProxyHub::AddExangeMap(const std::string &strID, boost::shared_ptr<ProxySession> pSession)
{
    boost::unique_lock<boost::shared_mutex> lock(m_ExangeMutex);

    auto itFind = m_ExangeMap.find(strID);
    if (m_ExangeMap.end() == itFind)
    {
        m_ExangeMap.insert(make_pair(strID, pSession));

        LOG_INFO_RLD("Exange map add id " << strID << " and map size is " << m_ExangeMap.size());

    }
    else
    {
        m_ExangeMap.erase(itFind); //ɾ���ϵ�

        m_ExangeMap.insert(make_pair(strID, pSession));

        LOG_INFO_RLD("Exange map delete old and add new id " << strID << " and map size is " << m_ExangeMap.size());
    }
    
}

bool ProxyHub::AddExangeMapZero(boost::shared_ptr<ProxySession> pSession, const std::string &strMsg)
{
    bool blRet = true;
    if (m_NeedAuth)
    {
        blRet = IsBussnessAuth((char*)strMsg.data(), strMsg.size());

        LOG_INFO_RLD("Auth result is " << blRet << " and msg is " << strMsg);

        if (blRet)
        {
            m_NeedAuth = false;
        }
    }
    else
    {
        blRet = m_NeedAuth;
    }

    if (blRet)
    {
        pSession->SetZeroID();
        std::string strZero = "0";
        AddExangeMap(strZero, pSession);

        LOG_INFO_RLD("Add exangemap zero id and msg is " << strMsg);        
    }

    return blRet;
}



void ProxyHub::SetAuthEnable(const bool blAuthEnable)
{
    m_blAuthEnable = blAuthEnable;

    if (m_blAuthEnable)
    {
        m_pMemCl = MemcacheClient::create();
        if (MemcacheClient::CACHE_SUCCESS != m_pMemCl->addServer(GetAuthSrcIP().c_str(), GetAuthSrcPort()))
        {
            MemcacheClient::destoy(m_pMemCl);
            m_pMemCl = NULL;
            m_blAuthEnable = false;

            LOG_ERROR_RLD("memcached client init failed, remote ip: " << m_strAuthSrcIP << ", remote port:" << m_iAuthSrcPort);
        }
        else
        {
            LOG_INFO_RLD("memcached client init succeed, remote ip: " << m_strAuthSrcIP << ", remote port:" << m_iAuthSrcPort);
        }
    }
}

bool ProxyHub::Auth(const std::string &strSrcID)
{
    if (!m_blAuthEnable || NULL == m_pMemCl)
    {
        LOG_INFO_RLD("current auth is disabled, so the result of auth client is true, client id is " << strSrcID);
        return true;
    }
    
    boost::unique_lock<boost::mutex>lock(m_MemClMutex);

    std::string strValue;
    bool blRet = MemcacheClient::CACHE_SUCCESS == m_pMemCl->get(strSrcID.c_str(), strValue);
    
    LOG_INFO_RLD("the result of auth client " <<strSrcID << " is " << (blRet ? "true" : "false"));

    return blRet;
}

char *ProxyHub::GeneratePackage(const std::string &strSrcID, const std::string &strDstID, const std::string &strType,
    const char *pContentBuffer, const boost::uint32_t uiContentBufferLen, boost::uint32_t &uiTotalLen)
{
    //RG,Len,src,dst,type,body,crc crc��ָ"src,dst,type,body"
    //RG,40,storage_upload_11111111111111111,0,0,0,0

    //content base64 encode
    const std::string &strContentEncoded = Encode64((const unsigned char*)pContentBuffer, uiContentBufferLen);
    const char *pContentBufferEncoded = strContentEncoded.data();
    const boost::uint32_t uiContentBufferLenEncoded = strContentEncoded.size();
    //LOG_INFO_RLD("=====uiContentBufferLenEncoded====: " << uiContentBufferLenEncoded);

    unsigned int uiSrcIDLen = strSrcID.length();
    unsigned int uiDestIDLen = strDstID.length();
    unsigned int uiTypeLen = strType.length();

    unsigned int uiToBeCrcBufferLen = (uiSrcIDLen + 1 + uiDestIDLen + 1 + uiTypeLen + 1 + uiContentBufferLenEncoded);

    //LOG_INFO_RLD("=====uiToBeCrcBufferLen====: " << uiToBeCrcBufferLen);

    unsigned short usCrc = 0;
    if (1)
    {
        char *pTobeCrcBuffer = new char[uiToBeCrcBufferLen + 1];
        snprintf(pTobeCrcBuffer, uiToBeCrcBufferLen + 1, "%s,%s,%s,%s", strSrcID.c_str(), strDstID.c_str(), strType.c_str(), pContentBufferEncoded);
        usCrc = crc16(pTobeCrcBuffer, uiToBeCrcBufferLen);
        delete[] pTobeCrcBuffer;
        pTobeCrcBuffer = NULL;
    }

    std::string strCrc = boost::lexical_cast<std::string>(usCrc);
    unsigned int uiPartEndLen = uiToBeCrcBufferLen + 1 + strCrc.length();

    //LOG_INFO_RLD("=====uiPartEndLen====: " << uiPartEndLen << " ===strCrc===" << strCrc.c_str());

    std::string strPartEndLen = boost::lexical_cast<std::string>(uiPartEndLen);
    unsigned int uiAllLen = 3 + strPartEndLen.length() + 1 + uiPartEndLen;

    //LOG_INFO_RLD("=====strPartEndLen====: " << strPartEndLen << " ===uiAllLen===" << uiAllLen);

    char *pAllBuffer = new char[uiAllLen+1];
    memset(pAllBuffer, 0, (uiAllLen+1));

    //ע�⣬����snprintf���һ���ֽڻ�Ĭ�����0������������Ҫ��һ
    snprintf(pAllBuffer, uiAllLen+1, "RG,%s,%s,%s,%s,%s,%s", strPartEndLen.c_str(), strSrcID.c_str(), strDstID.c_str(), strType.c_str(), pContentBufferEncoded, strCrc.c_str());

    uiTotalLen = uiAllLen;

    return pAllBuffer;
    
}

void ProxyHub::RemoveExangeMap(const std::string strID)
{
    boost::unique_lock<boost::shared_mutex> lock(m_ExangeMutex);

    auto itFind = m_ExangeMap.find(strID);
    if (m_ExangeMap.end() != itFind)
    {
        m_ExangeMap.erase(itFind);
        
        LOG_INFO_RLD("Remove exangemap id " << strID << " and map size is " << m_ExangeMap.size());

        if (strID == "0")
        {
            m_NeedAuth = true;
            LOG_INFO_RLD("Exangemap is empty and reset auth flag to true");
        }

        ////////
        //if (m_ExangeMap.empty())
        //{
        //    m_NeedAuth = true;
        //    LOG_INFO_RLD("Exangemap is empty and reset auth flag to true");
        //}
    }
    else
    {
        LOG_INFO_RLD("Remove exangemap id not equal,  current id " << strID << " and map size is " << m_ExangeMap.size());
    }

}

void ProxyHub::ProcessProtocol(boost::shared_ptr<std::list<std::string> > pProtoList,
    boost::shared_ptr<std::list<std::string> > pDstIDList,
    boost::shared_ptr<ProxySession> pSrcSession)
{
    auto itBegin = pDstIDList->begin();
    auto itEnd = pDstIDList->end();
    auto itBeginMsg = pProtoList->begin();
    while (itBegin != itEnd)
    {
        boost::shared_ptr<ProxySession> pDstSession;
        {
            boost::shared_lock<boost::shared_mutex> lock(m_ExangeMutex);
            ExangeMap::iterator itFindDst = m_ExangeMap.find(*itBegin);
            if (m_ExangeMap.end() != itFindDst)
            {
                pDstSession = itFindDst->second;

            }
        }

        if (NULL ==pDstSession.get())
        {
            char cPrint[64] = { 0 };
            boost::uint32_t uiSize = (*itBeginMsg).size() > sizeof(cPrint) ? sizeof(cPrint) : (*itBeginMsg).size();
            memcpy(cPrint, (*itBeginMsg).data(), uiSize);
            cPrint[uiSize] = (char)0;

            LOG_ERROR_RLD("Msg disgarded, src id is " << pSrcSession->GetID() << " dst id is " << (*itBegin) << " msg is " << cPrint);

            pDstIDList->erase(itBegin++);
            pProtoList->erase(itBeginMsg++);
            continue;
        }
        else
        {
            pDstSession->SyncWrite((char*)itBeginMsg->c_str(), itBeginMsg->size());

        }
        
        ++itBegin;
        ++itBeginMsg;
    }
    
    pSrcSession->UpdateWriteCallSnapshot();
    pSrcSession->BindRead();

}

bool ProxyHub::IsBussnessAuth(char *pPos, boost::uint32_t uiReadPos)
{
    char *pOriginalPos = pPos;

    boost::uint32_t uiCount = 0;
    while (1)
    {
        if (',' == *pPos++)
        {
            uiCount++;
            if (6 == uiCount)
            {

                if ('0' == *pPos && '0' == *(pPos - 2) && '0' == *(pPos - 4) && '0' == *(pPos - 6))
                {
                    return true;
                }

                return false;
            }
            
            if (pPos == (pOriginalPos + uiReadPos))
            {
                return false;
            }
        }
    }
}

ProxySession::ProxySession(ProxyHub &proxyhub, boost::shared_ptr<TCPSessionOfServer> pSession)
    : m_ProxyHub(proxyhub), m_Status(RUN), m_Func(NULL), m_pTCPSession(pSession), m_SeqNum(0), m_SrcIDReplaceByIncSeq(false), m_CreateSIDOnConnected(false),
    m_uiAsyncReadTimeOut(10), m_uiWriteCallNumbers(0), m_uiWriteCallSnapshot(0)
{
    LOG_INFO_RLD("Proxy session created");
}

ProxySession::~ProxySession()
{   
    LOG_INFO_RLD("Proxy session destroyed, and id is " << m_strID);

    if (!m_strZeroID.empty())
    {   
        LOG_INFO_RLD("Proxy session destroyed, and zero id is " << m_strZeroID);
    }
    
}

void ProxySession::ReadCB(boost::shared_ptr<TCPSessionOfServer> pSession,
    const boost::system::error_code &ec, std::size_t bytes_transferred, void *pValue)
{

    char *pBuffer = (char*)pValue;

    if (ec)
    {
        LOG_ERROR_RLD("ReadCB receive error msg: " << ec.message() << " id is " << m_strID << " zero id is " << m_strZeroID);

        SetStatus(STOP);

        delete[] pBuffer;
        pBuffer = NULL;

        m_Func = NULL;

        m_ProxyHub.RemoveExangeMap(m_strID);
        if (!m_strZeroID.empty())
        {
            m_ProxyHub.RemoveExangeMap(m_strZeroID);
        }
                
        LOG_ERROR_RLD("Remove session by id " << m_strID << " and zero id " << m_strZeroID);
        return;

    }

    boost::uint32_t uiReadPos = *(boost::uint32_t*)pBuffer;

    uiReadPos += bytes_transferred;
    *(boost::uint32_t*)pBuffer = uiReadPos;

    char *pUsedBuffer = pBuffer + sizeof(boost::uint32_t);
    boost::uint32_t uiUsedBufferSize = BUFFER_SIZE - sizeof(boost::uint32_t);

    std::list<std::string> *pProtoList = new std::list < std::string > ;
    std::list<std::string> *pDstIDList = new std::list < std::string >;
    
    boost::uint32_t uiTmpSize = uiReadPos;
    char *pBufferNewPos = pUsedBuffer;
    while (1)
    {
        boost::uint32_t uiFlag = 0;
        bool blRet = ParseProtocol(pBufferNewPos, uiTmpSize, pProtoList, uiFlag, pBufferNewPos, pDstIDList);
        if (uiFlag)
        {
            uiTmpSize = *(boost::uint32_t *)(pBufferNewPos - sizeof(boost::uint32_t));//*(boost::uint32_t*)pBuffer;

        }
        else
        {
            if (blRet)
            {
                *(boost::uint32_t*)pBuffer = 0;
            }
            else
            {
                if (!pProtoList->empty())
                {
                    char *pStuff = new char[uiTmpSize];
                    memcpy(pStuff, pBufferNewPos, uiTmpSize);

                    memcpy(pUsedBuffer, pStuff, uiTmpSize);
                    *(boost::uint32_t*)pBuffer = uiTmpSize;

                    delete[] pStuff;
                    pStuff = NULL;
                }
                else
                {
                    //LOG_INFO_RLD("Need continue to reading.");
                }
                
            }

            break;
        }
    }
    
 
    uiReadPos = *(boost::uint32_t*)pBuffer;

    if (!pProtoList->empty())
    {
        
        char *pBf = pUsedBuffer + uiReadPos;
        boost::uint32_t uiSz = uiUsedBufferSize - uiReadPos;

        m_Func = boost::bind(&TCPSessionOfServer::AsyncRead, pSession, pBf, uiSz, m_uiAsyncReadTimeOut, (void *)pBuffer, 0);

        m_ProxyHub.ProcessProtocol(boost::shared_ptr<std::list<std::string> >(pProtoList),
            boost::shared_ptr<std::list<std::string> >(pDstIDList),
            shared_from_this());

    }
    else
    {
        delete pProtoList;
        pProtoList = NULL;

        delete pDstIDList;
        pDstIDList = NULL;

        UpdateWriteCallSnapshot();
        pSession->AsyncRead((pUsedBuffer + uiReadPos), (uiUsedBufferSize - uiReadPos), m_uiAsyncReadTimeOut, (void *)pBuffer);
    }


}

void ProxySession::WriteCB(boost::shared_ptr<TCPSessionOfServer> pSession,
    const boost::system::error_code &ec, std::size_t bytes_transferred, void *pValue)
{
    //////
    //if (ec)
    //{
    //    LOG_ERROR_RLD("WriteCB receive error msg: " << ec.message() << " id is " << m_strID << " zero id is " << m_strZeroID);

    //    SetStatus(STOP);

    //    m_Func = NULL;

    //    m_ProxyHub.RemoveExangeMap(m_strID);
    //    if (!m_strZeroID.empty())
    //    {
    //        m_ProxyHub.RemoveExangeMap(m_strZeroID);
    //    }

    //    m_SendingMutex.unlock();
    //    
    //    return;
    //}

    //m_SendingMutex.unlock();

}


bool ProxySession::ReadTimeoutCB(boost::shared_ptr<TCPSessionOfServer> pSession)
{
    bool IsContinueTimeout = true;
    //IsContinueTimeoutΪfalse�����ʾ�ڶ���ʱ���ʱ���ڣ�û�з���д�Ķ���������Ϊ��Session����ĳ�ʱ��
    if (m_uiWriteCallSnapshot < m_uiWriteCallNumbers)
    {
        IsContinueTimeout = true;
    }
    else
    {
        IsContinueTimeout = false;
    }

    LOG_INFO_RLD("Read timeout and continue flag is " << (IsContinueTimeout ? "true" : "false") << ", m_uiWriteCallSnapshot is " << m_uiWriteCallSnapshot
        << ", m_uiWriteCallNumbers is " << m_uiWriteCallNumbers << " remote address is " << pSession->GetSocket().remote_endpoint().address().to_string());

    UpdateWriteCallSnapshot(); //Reset snapshot number

    return IsContinueTimeout;
}

bool ProxySession::ParseProtocol(char *pBuffer, const boost::uint32_t uiSize, std::list<std::string> *pstrProtoList,
    boost::uint32_t &uiFlag, char *&pBufferNewPos, std::list<std::string> *pDstIDList)
{

    if (3 > uiSize)
    {
        LOG_ERROR_RLD("Parse protocol buffer size not enought, size is " << uiSize);
        return false;
    }

    boost::uint32_t uiStep = 0;
    if ('R' != *pBuffer || 'G' != *(pBuffer + 1))
    {
        LOG_ERROR_RLD("Parse protocol buffer not begin with RG" );
        return false;
    }

    uiStep += 3;
    if (uiSize <= (uiStep + 1))
    {
        LOG_ERROR_RLD("Parse protocol buffer size not enought, size is " << uiSize);
        return false;
    }

    boost::uint32_t uiLen = 0;
    std::string strSrcID;
    std::string strDstID;
    std::string strValue;
    while (1)
    {
        char cValue = *(pBuffer + uiStep);
        if (',' != cValue)
        {
            strValue += cValue;
        }
        else
        {
            //break;
            uiLen = atoi(strValue.c_str());

            if (uiSize < (uiStep + 1 + uiLen))
            {
                //LOG_ERROR_RLD("Parse protocol buffer error, size of buffer is " << uiSize << " and protocol size is " << (uiStep + 1 + uiLen));
                return false;
            }

            char *pTmp = pBuffer + uiStep + 1;
            boost::uint32_t uiIDCn = 0;
            while (1)
            {
                if (0 == uiIDCn)
                {
                    strSrcID += *pTmp++;
                }
                else
                {
                    strDstID += *pTmp++;
                }

                if (',' == *pTmp)
                {
                    ++uiIDCn;
                    if (2 <= uiIDCn)
                    {
                        break;
                    }
                    ++pTmp;
                }
            }
            break;            
        }

        uiStep += 1;
        if (uiSize <= (uiStep + 1))
        {
            LOG_ERROR_RLD("Parse protocol buffer not enought, size is " << uiSize << " and protocol size is " << (uiStep + 1));
            return false;
        }
    }

    uiStep += uiLen;
    
    std::string strProto;
    strProto.assign(pBuffer, (uiStep + 1));

    PreprocessProtoMsg(strProto, pDstIDList, strSrcID, strDstID);
    
    pstrProtoList->push_back(strProto);
    

    if (uiSize == (uiStep + 1))
    {
        //memset((pBuffer - sizeof(boost::uint32_t)), 0, BUFFER_SIZE);
        //printf("buffer clean.\n");
        return true;
    }

    ///////
    boost::uint32_t uiTmpSize = uiSize - uiStep - 1;

    pBufferNewPos = pBuffer + uiStep + 1;
    *(boost::uint32_t *)(pBufferNewPos - sizeof(boost::uint32_t)) = uiTmpSize;

    uiFlag = 1;

    return false;
}


void ProxySession::SyncWrite(char *pInputBuffer, const boost::uint32_t uiBufferSize)
{
    boost::unique_lock<boost::mutex> lock(m_SendingMutex);
    if (STOP == GetStatus())
    {        
        LOG_ERROR_RLD("SyncWrite not allow and return by status is STOP, id is " << m_strID);
        return;
    }

    if (!m_pTCPSession.expired())
    {
        boost::shared_ptr<TCPSessionOfServer> tcpSession = m_pTCPSession.lock();
        if (NULL != tcpSession.get())
        {
            ++m_uiWriteCallNumbers;

            //tcpSession->AsyncWrite(pInputBuffer, uiBufferSize, NULL);
            boost::uint32_t uiWrited = 0;
            std::string strErrorMsg;
            if (!tcpSession->SyncWrite(pInputBuffer, uiBufferSize, uiWrited, strErrorMsg))
            {
                SetStatus(STOP);
                ///////
                //m_Func = NULL;

                //m_ProxyHub.RemoveExangeMapPost(m_strID);
                //if (!m_strZeroID.empty())
                //{
                //    m_ProxyHub.RemoveExangeMapPost(m_strZeroID);
                //}
                
                LOG_ERROR_RLD("SyncWrite error : " << strErrorMsg << " id is " << m_strID);
            }
            else
            {
                ++m_uiWriteCallNumbers;
            }
        }
        else
        {
            LOG_ERROR_RLD("SyncWrite error, can not valid share pointer" << " id is " << m_strID);
        }        
    }

}

void ProxySession::SetID(const std::string &strID)
{
    m_strID = strID;
}

const std::string& ProxySession::GetID()
{
    return m_strID;
}

void ProxySession::BindRead()
{
    m_Func();    
}

void ProxySession::SetSeqID(const boost::uint64_t uiSeq)
{
    m_SeqNum = uiSeq;

    char cTmp[512] = { 0 };
    snprintf(cTmp, sizeof(cTmp), "%llu", (unsigned long long)m_SeqNum);
    m_strSeqNum = cTmp;
}

boost::uint64_t ProxySession::GetSeqID()
{
    return m_SeqNum;
}

void ProxySession::PreprocessProtoMsg(std::string &strProto, std::list<std::string> *pDstIDList, const std::string &strSrcID, const std::string &strDstID)
{
    if (strProto == "RG,9,0,0,0,0,0")
    {
        pDstIDList->push_back(m_strID); //shake msg , only send back.

        LOG_INFO_RLD("Receive shake hand msg " << strProto << " dst id is " << strSrcID << " src id is " << m_strID << " src zeroid is " << m_strZeroID);
    }
    else
    {
        bool IsSessionMsg = false;
        bool IsZeroPeer = false;
        if (m_strID.empty())
        {
            const std::string &strToAuthID = m_SrcIDReplaceByIncSeq ? m_strSeqNum : strSrcID;
            
            if (!Auth(strToAuthID))
            {
                return;
            }

            m_strID = strToAuthID;

            m_ProxyHub.AddExangeMap(m_strID, shared_from_this());
            IsZeroPeer = m_ProxyHub.AddExangeMapZero(shared_from_this(), strProto);
            if (IsZeroPeer)
            {
                m_CreateSIDOnConnected = false;
            }

            LOG_INFO_RLD("Add src id " << m_strID << " original src id is " << strSrcID);
        }

        if (m_CreateSIDOnConnected && !IsZeroPeer)
        {
            IsSessionMsg = true;

            ////
            //char pUUID[32] = { 0 }; //ע�⣬����UUID������0��β����Ҫ���Ƶ�string����ȥ��
            //GenerateUUID(pUUID, sizeof(pUUID), (unsigned int)m_SeqNum);
            //std::string strUUID;
            //strUUID.assign(pUUID, sizeof(pUUID));
            const std::string &strUUID = CreateUUID();

            m_ProxyHub.AddExangeMap(strUUID, shared_from_this());
            m_ProxyHub.RemoveExangeMap(m_strID);
            m_strID = strUUID;

            Json::Value jsBody;
            jsBody["VER"] = "1.0";
            jsBody["CMD"] = "RESPON";
            jsBody["SEQ"] = "";
            jsBody["M5"] = "";
            jsBody["MSG"] = "0";
            jsBody["TYPE"] = "LOGIN";
            jsBody["ST"] = "0";
            jsBody["SESSION"] = strUUID; //pUUID; //"UUID";
            jsBody["UUID"] = strUUID; //pUUID; //"";
            jsBody["KS"] = "";

            Json::FastWriter fastwriter;                        
            const std::string &strBody = fastwriter.write(jsBody);//jsBody.toStyledString();
            unsigned int uiMsgLen = 0;
            boost::shared_ptr<char> pMsg(m_ProxyHub.GeneratePackage("0", strSrcID, "1", strBody.data(), strBody.size(), uiMsgLen));
            strProto.clear();
            strProto.assign(pMsg.get(), uiMsgLen);

            m_CreateSIDOnConnected = false;

            LOG_INFO_RLD("Create session msg is " << pMsg.get());

            pDstIDList->push_back(m_strID); //����Sessionʱ����Ҫ������Ϣ�������
        }
        else
        {
            pDstIDList->push_back(strDstID);
        }

        //RG,����,Դ��,Ŀ���,����, Э����,У���
        if (m_SrcIDReplaceByIncSeq && !IsSessionMsg)
        {
            std::string::size_type pos = strProto.find(',', 3);
            if (std::string::npos == pos)
            {
                LOG_ERROR_RLD("Failed to convert proto msg : " << strProto);
            }
            std::string strEnd = strProto.substr(pos + 1); //not include ','    src,dst,....
            std::string strFront; // = strProto.substr(0, pos); // not include ','   rg,len

            pos = strEnd.find(',');
            if (std::string::npos == pos)
            {
                LOG_ERROR_RLD("Failed to convert proto msg : " << strProto);
            }
            std::string strEnd2 = strEnd.substr(pos);
            strEnd2 = m_strSeqNum + strEnd2;

            char cLen[256] = { 0 };
            snprintf(cLen, sizeof(cLen), "RG,%u,", (unsigned int)strEnd2.size());
            strFront = cLen;

            strProto = strFront + strEnd2;
        }

        LOG_INFO_RLD("Proto msg is " << strProto.substr(0, 64) << " srcid replace flag is " << m_SrcIDReplaceByIncSeq << " session msg flag is " << IsSessionMsg);
    }
}

bool ProxySession::Auth(const std::string &strSrcID)
{
    if (!m_ProxyHub.Auth(strSrcID))
    {
        if (!m_pTCPSession.expired())
        {
            boost::shared_ptr<TCPSessionOfServer> tcpSession = m_pTCPSession.lock();
            if (NULL != tcpSession.get())
            {
                m_ProxyHub.GetAuthRunner().Post(boost::bind(&TCPSessionOfServer::Close, tcpSession));
                //tcpSession->Close();
                return false;
            }
        }
        else
        {
            LOG_ERROR_RLD("the tcp session that in the proxy session is expired, so auth is true");
        }
    }

    return true;
}


