// 解析Http头部请求信息 
#ifndef _ANALYZE_REQUEST_HEADER_DATA_HEAD_
#define _ANALYZE_REQUEST_HEADER_DATA_HEAD_
#include <WinSock2.h>
#define MFREE(_a)  if(_a){free(_a); _a=NULL;}

typedef struct _key_value 
{
    char* key;
    char* value;
}kvp, *pkvp;

class RequestHeaderInfo
{
public:
    RequestHeaderInfo(SOCKET s);
    ~RequestHeaderInfo();

public:
    const char* GetAcceptLanguage()
    {
        return m_accept_language;
    }
    const char* GetAcceptEncoding()
    {
        return m_accept_encoding;
    }
    const char* GetRequestFile()
    {
        return m_request_file;
    }
    const char* GetCookie()
    {
        return m_cookies;
    }
    const char* GetPostData()
    {
        return m_post_data;
    }
    const char* GetUserAgent()
    {
        return m_user_agent;
    }
    const char* GetContextType()
    {
        return m_content_type;
    }

protected:
    // 解析数据 
    char* m_linedata;
    char* m_key;
    char* m_value;

    unsigned int GetLine(SOCKET s);  // 获取一行数据 
    bool GetCompare(SOCKET s);  // 获取一行并解析成key-value 对 
    bool AnalyzeHttpData(SOCKET s);  // 接收头部所有数据并解析 

private:
    // POST 请求的数据 
    bool m_ispost;
    unsigned long m_data_len;
    char* m_post_data;

    // POST 或者 GET 跟的对象 
    char* m_request_file;

    // 其他参数数据 
    char* m_accept_language;
    char* m_accept_encoding;
    char* m_content_type;
    char* m_user_agent;
    char* m_cookies;
    char* m_accept;
};


#endif //  _ANALYZE_REQUEST_HEADER_DATA_HEAD_