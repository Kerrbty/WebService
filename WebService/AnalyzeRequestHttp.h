// 解析Http头部请求信息 
#ifndef _ANALYZE_REQUEST_HEADER_DATA_HEAD_
#define _ANALYZE_REQUEST_HEADER_DATA_HEAD_
#include <WinSock2.h>
#include "List.h"
#define MALLC(_size) malloc(_size)
#define MFREE(_a)  if(_a){free(_a); _a=NULL;}

typedef struct _key_value 
{
    LIST_ENTRY next;
    char* key;
    char* value;
}kvp, *pkvp;

class RequestHeaderInfo
{
public:
    RequestHeaderInfo(SOCKET s);
    ~RequestHeaderInfo();

public:
    // 文件路径 和 参数 
    const char* GetRequestFile()
    {
        return m_request_path;
    }
    const char* GetCommandLine()
    {
        return m_request_command;
    }

    // 各种头部数据 
    const char* GetHeader(const char* key);
    const char* GetAcceptLanguage()
    {
        return GetHeader("Accept-Language");
    }
    const char* GetAcceptEncoding()
    {
        return GetHeader("Accept-Encoding");
    }
    const char* GetCookie()
    {
        return GetHeader("Cookie");
    }
    const char* GetUserAgent()
    {
        return GetHeader("User-Agent");
    }
    const char* GetContextType()
    {
        return GetHeader("Content-Type");
    }

    // 附加数据 
    unsigned long GetHeadContentLength()
    {
        return m_data_len;
    }
    const char* GetHeadContentData()
    {
        return m_post_data;
    }

protected:
    // 解析数据 
    char* m_linedata;
    char* m_key;
    char* m_value;
    kvp m_head_compare;

    unsigned int GetLine();  // 获取一行数据 
    bool GetCompare();  // 获取一行并解析成key-value 对 
    bool AnalyzeRequestData();  // 接收头部所有数据并解析 
    bool AnalyzeMethod();  // HTTP 头部，第一行方法,路径,协议 
    void AnalyzeHeadPair();  // HTTP 头部键值对 
    unsigned long AnalyzeHeadContent(); // HTTP 头部附加信息 

private:
    // SOKCET 接口 
    SOCKET m_client_socket;

    // POST 请求的数据 
    bool m_ispost;
    unsigned long m_data_len;
    char* m_post_data;

    // POST 或者 GET 跟的对象 
    char* m_request_path;  // URL 路径 
    char* m_request_command; // URL中的请求参数 
};


#endif //  _ANALYZE_REQUEST_HEADER_DATA_HEAD_