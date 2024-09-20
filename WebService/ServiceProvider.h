// 服务提供者，用来解析用户请求，并返回用户信息 
#ifndef _SERVICE_PROVIDER_HEADER_
#define _SERVICE_PROVIDER_HEADER_
#include "AnalyzeRequestHttp.h"

#ifdef _cplusplus
extern "C" {
#endif // _cplusplus 

bool ResponseData(SOCKET s, RequestHeaderInfo *request);

#ifdef _cplusplus
};
#endif // _cplusplus 

#endif // _SERVICE_PROVIDER_HEADER_ 