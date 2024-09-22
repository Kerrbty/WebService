#ifndef _WEB_SERVER_COMMON_HEADER_HH_H_
#define _WEB_SERVER_COMMON_HEADER_HH_H_

#ifdef _cplusplus
extern "C" {
#endif // _cplusplus 

// �ַ���תBYTE 
unsigned char make_byte(const char* pbuf);

// ����URLδ����GBK���� 
char* UrlDecode(const char* src, unsigned int srclen, char* dst, unsigned int dstlen);

// ���ݼ���sha1 
bool calcBufferSha1(unsigned char* buffer, unsigned long len, unsigned char out[20]);

// ���������Ƿ��� utf-8 ���� 
bool isUTF8(const void* data, unsigned int length) ;

#ifdef _cplusplus
};
#endif // _cplusplus 

#endif // _WEB_SERVER_COMMON_HEADER_HH_H_ 