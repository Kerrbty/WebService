#ifndef _WEB_SERVER_COMMON_HEADER_HH_H_
#define _WEB_SERVER_COMMON_HEADER_HH_H_

#ifdef _cplusplus
extern "C" {
#endif // _cplusplus 

// 字符串转BYTE 
unsigned char make_byte(const char* pbuf);

// 解码URL未本地GBK编码 
char* UrlDecode(const char* src, unsigned int srclen, char* dst, unsigned int dstlen);

// 数据计算sha1 
bool calcBufferSha1(unsigned char* buffer, unsigned long len, unsigned char out[20]);

// 数据内容是否是 utf-8 编码 
bool isUTF8(const void* data, unsigned int length) ;

#ifdef _cplusplus
};
#endif // _cplusplus 

#endif // _WEB_SERVER_COMMON_HEADER_HH_H_ 