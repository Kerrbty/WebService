// ����Httpͷ��������Ϣ 
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
    // �������� 
    char* m_linedata;
    char* m_key;
    char* m_value;

    unsigned int GetLine(SOCKET s);  // ��ȡһ������ 
    bool GetCompare(SOCKET s);  // ��ȡһ�в�������key-value �� 
    bool AnalyzeHttpData(SOCKET s);  // ����ͷ���������ݲ����� 

private:
    // POST ��������� 
    bool m_ispost;
    unsigned long m_data_len;
    char* m_post_data;

    // POST ���� GET ���Ķ��� 
    char* m_request_file;

    // ������������ 
    char* m_accept_language;
    char* m_accept_encoding;
    char* m_content_type;
    char* m_user_agent;
    char* m_cookies;
    char* m_accept;
};


#endif //  _ANALYZE_REQUEST_HEADER_DATA_HEAD_