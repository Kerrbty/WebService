#ifndef _WEB_SERVER_COMMON_HEADER_HH_H_
#define _WEB_SERVER_COMMON_HEADER_HH_H_

#ifdef _cplusplus
extern "C" {
#endif // _cplusplus 

// �ַ���תBYTE 
unsigned char make_byte(const char* pbuf);

// ����URLδ����GBK���� 
char* UrlDecode(const char* src, unsigned int srclen, char* dst, unsigned int dstlen);

#ifdef _cplusplus
};
#endif // _cplusplus 

#endif // _WEB_SERVER_COMMON_HEADER_HH_H_ 