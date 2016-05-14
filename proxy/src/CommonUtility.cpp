#include "CommonUtility.h"
#include "uuid.h"


CommonUtility::CommonUtility()
{
}


CommonUtility::~CommonUtility()
{
}

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


std::string CreateUUID()
{
    typedef struct _GUID
    {
        unsigned int  Data1;
        unsigned short Data2;
        unsigned short Data3;
        unsigned char  Data4[8];
    } GUID_ST;

    char buf[64] = { 0 };
    GUID_ST guid;

    uuid_generate(reinterpret_cast<unsigned char *>(&guid));

    snprintf(
        buf,
        sizeof(buf),
        "%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1],
        guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5],
        guid.Data4[6], guid.Data4[7]);

    return std::string(buf);
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