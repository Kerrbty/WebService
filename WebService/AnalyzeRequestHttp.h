// ����Httpͷ��������Ϣ 
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
    // �ļ�·�� �� ���� 
    const char* GetRequestFile()
    {
        return m_request_path;
    }
    const char* GetCommandLine()
    {
        return m_request_command;
    }

    // ����ͷ������ 
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

    // �������� 
    unsigned long GetHeadContentLength()
    {
        return m_data_len;
    }
    const char* GetHeadContentData()
    {
        return m_post_data;
    }

protected:
    // �������� 
    char* m_linedata;
    char* m_key;
    char* m_value;
    kvp m_head_compare;

    unsigned int GetLine();  // ��ȡһ������ 
    bool GetCompare();  // ��ȡһ�в�������key-value �� 
    bool AnalyzeRequestData();  // ����ͷ���������ݲ����� 
    bool AnalyzeMethod();  // HTTP ͷ������һ�з���,·��,Э�� 
    void AnalyzeHeadPair();  // HTTP ͷ����ֵ�� 
    unsigned long AnalyzeHeadContent(); // HTTP ͷ��������Ϣ 

private:
    // SOKCET �ӿ� 
    SOCKET m_client_socket;

    // POST ��������� 
    bool m_ispost;
    unsigned long m_data_len;
    char* m_post_data;

    // POST ���� GET ���Ķ��� 
    char* m_request_path;  // URL ·�� 
    char* m_request_command; // URL�е�������� 
};


#endif //  _ANALYZE_REQUEST_HEADER_DATA_HEAD_