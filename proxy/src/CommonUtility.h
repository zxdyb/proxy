#ifndef _COMMON_UTILITY_
#define _COMMON_UTILITY_

#include <list>
#include <string>

class CommonUtility
{
public:
    CommonUtility();
    ~CommonUtility();
};

void GenerateUUID(char *pUUIDBuffer, const unsigned int uiBufferSize, const unsigned int uiSeed);

std::string CreateUUID();

unsigned short crc16(const char* data, int length);

std::string Encode64(unsigned char const* bytes_to_encode, unsigned int in_len);

std::string Decode64(unsigned char const* encoded_string, unsigned int in_len);

#endif