#include "ProxyServer.h"
#include "LogRLD.h"
#include "json/json.h"
#include "boost/bind.hpp"
#include "boost/lexical_cast.hpp"

#ifdef WIN32
#ifndef snprintf
#define snprintf _snprintf
#endif
#endif

void GenerateUUID(char *pUUIDBuffer, const unsigned int uiBufferSize, const unsigned int uiSeed)
{
    //unsigned int uiTid = (unsigned int)boost::this_thread::get_id();
    //srand((unsigned)time(NULL));
    srand(uiSeed);
    const unsigned int uiSize = uiBufferSize; /// sizeof(wchar_t);

    for (unsigned int i = 0; i < uiSize; i++)
    {
        pUUIDBuffer[i] = rand() % ('z' - 'a' + 1) + 'a'; //生成要求范围内的随机数。
    }

    //pUUIDBuffer[uiSize - 1] = 0;
}


static const short g_table[256] = {
    0, -32763, -32753, 10, -32741, 30, 20, -32751, -32717, 54, 60, -32711, 40, -32723, -32729, 34,
    -32669, 102, 108, -32663, 120, -32643, -32649, 114, 80, -32683, -32673, 90, -32693, 78, 68, -32703,
    -32573, 198, 204, -32567, 216, -32547, -32553, 210, 240, -32523, -32513, 250, -32533, 238, 228, -32543,
    160, -32603, -32593, 170, -32581, 190, 180, -32591, -32621, 150, 156, -32615, 136, -32627, -32633, 130,
    -32381, 390, 396, -32375, 408, -32355, -32361, 402, 432, -32331, -32321, 442, -32341, 430, 420, -32351,
    480, -32283, -32273, 490, -32261, 510, 500, -32271, -32301, 470, 476, -32295, 456, -32307, -32313, 450,
    320, -32443, -32433, 330, -32421, 350, 340, -32431, -32397, 374, 380, -32391, 360, -32403, -32409, 354,
    -32477, 294, 300, -32471, 312, -32451, -32457, 306, 272, -32491, -32481, 282, -32501, 270, 260, -32511,
    -31997, 774, 780, -31991, 792, -31971, -31977, 786, 816, -31947, -31937, 826, -31957, 814, 804, -31967,
    864, -31899, -31889, 874, -31877, 894, 884, -31887, -31917, 854, 860, -31911, 840, -31923, -31929, 834,
    960, -31803, -31793, 970, -31781, 990, 980, -31791, -31757, 1014, 1020, -31751, 1000, -31763, -31769, 994,
    -31837, 934, 940, -31831, 952, -31811, -31817, 946, 912, -31851, -31841, 922, -31861, 910, 900, -31871,
    640, -32123, -32113, 650, -32101, 670, 660, -32111, -32077, 694, 700, -32071, 680, -32083, -32089, 674,
    -32029, 742, 748, -32023, 760, -32003, -32009, 754, 720, -32043, -32033, 730, -32053, 718, 708, -32063,
    -32189, 582, 588, -32183, 600, -32163, -32169, 594, 624, -32139, -32129, 634, -32149, 622, 612, -32159,
    544, -32219, -32209, 554, -32197, 574, 564, -32207, -32237, 534, 540, -32231, 520, -32243, -32249, 514
};

unsigned short crc16(const char* data, int length)
{
    unsigned int crc = 0;
    for (int i = 0; i < length; i++)
    {
        crc = ((crc & 0xFF) << 8)
            ^ g_table[(((crc & 0xFF00) >> 8) ^ data[i]) & 0xFF];
    }
    crc = crc & 0xFFFF;

    return (unsigned short)crc;
}


static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static inline bool is_base64(unsigned char c) 
{
    return (isalnum(c) || (c == '+') || (c == '/'));
}
//编码
//DataByte
//    [in]输入的数据长度,以字节为单位
//
std::string Encode64(unsigned char const* bytes_to_encode, unsigned int in_len)
{
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';

    }

    return ret;
}

//解码
//DataByte
//    [in]输入的数据长度,以字节为单位
//OutByte
//    [out]输出的数据长度,以字节为单位,请不要通过返回值计算
//    输出数据的长度
std::string Decode64(unsigned char const* encoded_string, unsigned int in_len)
{
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }

    return ret;
}


ProxyHub::ProxyHub(const unsigned short uiPort) : m_TSvr(uiPort), m_NeedAuth(true), m_SeqNum(0), m_SrcIDReplaceByIncSeq(false), m_CreateSIDOnConnected(false)
{

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
    //if (m_SrcIDReplaceByIncSeq) //确保都包含了递增的序列号，可能后面的业务会用到这个资源
    {        
        pProxySession->SetSeqID(++m_SeqNum);
    }
    pProxySession->SetCreateSIDOnConnected(m_CreateSIDOnConnected);

    LOG_INFO_RLD("Accept connection and remote address is " << strAddress.c_str() << " and seq id is " << pProxySession->GetSeqID());
    
    pSession->SetCallBack(boost::bind(&ProxySession::ReadCB, pProxySession, _1, _2, _3, _4), true);
    //pSession->SetCallBack(boost::bind(&ProxySession::WriteCB, pProxySession, _1, _2, _3, _4), false);

    char *pBuffer = new char[BUFFER_SIZE];
    memset(pBuffer, 0, BUFFER_SIZE);

    pSession->AsyncRead(pBuffer + sizeof(boost::uint32_t), BUFFER_SIZE - sizeof(boost::uint32_t), (void *)pBuffer);


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
        m_ExangeMap.erase(itFind); //删除老的

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



char *ProxyHub::GeneratePackage(const std::string &strSrcID, const std::string &strDstID, const std::string &strType,
    const char *pContentBuffer, const boost::uint32_t uiContentBufferLen, boost::uint32_t &uiTotalLen)
{
    //RG,Len,src,dst,type,body,crc crc是指"src,dst,type,body"
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
        char *pTobeCrcBuffer = new char[uiToBeCrcBufferLen];
        snprintf(pTobeCrcBuffer, uiToBeCrcBufferLen, "%s,%s,%s,%s", strSrcID.c_str(), strDstID.c_str(), strType.c_str(), pContentBufferEncoded);
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

    //注意，这里snprintf最后一个字节会默认填充0，所以这里需要加一
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

        if (m_ExangeMap.empty())
        {
            m_NeedAuth = true;
            LOG_INFO_RLD("Exangemap is empty and reset auth flag to true");
        }
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
    : m_ProxyHub(proxyhub), m_Status(RUN), m_Func(NULL), m_pTCPSession(pSession), m_SeqNum(0), m_SrcIDReplaceByIncSeq(false), m_CreateSIDOnConnected(false)
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

        m_Func = boost::bind(&TCPSessionOfServer::AsyncRead, pSession, pBf, uiSz, (void *)pBuffer, 0);

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

        pSession->AsyncRead((pUsedBuffer + uiReadPos), (uiUsedBufferSize - uiReadPos), (void *)pBuffer);
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

            m_strID = m_SrcIDReplaceByIncSeq ? m_strSeqNum : strSrcID;
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

            char pUUID[32] = { 0 }; //注意，这里UUID不是以0结尾，需要复制到string类中去。
            GenerateUUID(pUUID, sizeof(pUUID), (unsigned int)m_SeqNum);
            std::string strUUID;
            strUUID.assign(pUUID, sizeof(pUUID));

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

            const std::string &strBody = jsBody.toStyledString();
            unsigned int uiMsgLen = 0;
            boost::shared_ptr<char> pMsg(m_ProxyHub.GeneratePackage("0", strSrcID, "1", strBody.data(), strBody.size(), uiMsgLen));
            strProto.clear();
            strProto.assign(pMsg.get(), uiMsgLen);

            m_CreateSIDOnConnected = false;

            LOG_INFO_RLD("Create session msg is " << pMsg.get());

            pDstIDList->push_back(m_strID); //创建Session时，需要返回消息给接入端
        }
        else
        {
            pDstIDList->push_back(strDstID);
        }

        //RG,长度,源端,目标端,类型, 协议体,校验和
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
