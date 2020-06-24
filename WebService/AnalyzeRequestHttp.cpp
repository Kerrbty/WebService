#include "AnalyzeRequestHttp.h"
#include <stdio.h>
#define START_SIZE  32

RequestHeaderInfo::RequestHeaderInfo(SOCKET s)
{
    m_linedata = NULL;
    m_key = NULL;
    m_value = NULL;

    m_ispost = false;
    m_data_len = 0;
    m_post_data = NULL;

    m_request_file = NULL;
    m_request_command = NULL;

    m_accept_language = NULL;
    m_accept_encoding = NULL;
    m_content_type = NULL;
    m_user_agent = NULL;
    m_cookies = NULL;
    m_accept = NULL;

    AnalyzeHttpData(s);
}

RequestHeaderInfo::~RequestHeaderInfo()
{
    MFREE(m_linedata);
    MFREE(m_key);
    MFREE(m_value);
    MFREE(m_post_data);
    MFREE(m_request_file);
    MFREE(m_accept_language);
    MFREE(m_accept_encoding);
    MFREE(m_content_type);
    MFREE(m_user_agent);
    MFREE(m_cookies);
    MFREE(m_accept);
}

// ��ȡͷ�������һ������ 
unsigned int RequestHeaderInfo::GetLine(SOCKET s)
{
    unsigned int i = 0;
    unsigned int uisize = START_SIZE;
    char *szstr = (char*)malloc(uisize);
    memset(szstr, 0, uisize);

    MFREE(m_linedata);
    while(szstr)
    {
        while (i<uisize)
        {
            recv(s, szstr+i, 1, 0);
            if (szstr[i] == '\n')
            {
                szstr[i] = '\0';
                m_linedata = szstr;
                return i;
            }
            else if (szstr[i] == '\r')
            {
                szstr[i] = '\0';
            }
            else
            {
                i++;
            }
        }
        char* tmp = (char*)malloc(uisize*2);
        if (tmp)
        {
            memset(tmp, 0, uisize*2);
            memcpy(tmp, szstr, uisize);
        }
        MFREE(szstr);
        szstr = tmp;
        uisize = uisize*2;
    }
    return 0;
}

// ��ȡͷ�������һ����ֵ�� 
bool RequestHeaderInfo::GetCompare(SOCKET s)
{
    unsigned int counts = GetLine(s);
    if (counts==0)
    {
        return false;
    }

    const char* p = strchr(m_linedata, ':');
    if (p == NULL)
    {
        return false;
    }

    MFREE(m_key);
    MFREE(m_value);
    m_key = (char*)malloc(counts+1);
    m_value = (char*)malloc(counts+1);
    if (m_key && m_value)
    {
        memset(m_key, 0, counts+1);
        memset(m_value, 0, counts+1);
        memcpy(m_key, m_linedata, p-m_linedata);
        strcpy(m_value, p+2);
        return true;
    }

    MFREE(m_key);
    MFREE(m_value);
    return false;
}

// ������������ 
bool RequestHeaderInfo::AnalyzeHttpData(SOCKET s)
{
    bool bret = false; 
    // ��ȡ��һ�п��� GET ���� POST 
    unsigned int uiLen = GetLine(s); 
    if (uiLen!=0)
    {
        if ( memicmp(m_linedata, "GET", 3) == 0 )
        {
            m_ispost = false;
        }
        else if (memicmp(m_linedata, "POST", 4) == 0)
        {
            m_ispost = true;
            m_data_len = 0;
            MFREE(m_post_data);
        }

        int skip = 0;
        // �Թ� GET ���� POST ������������ 
        while(m_linedata[skip] != ' ' && m_linedata[skip] != '\0')
        {
            skip++;
        }
        // �Թ��ո� 
        while(m_linedata[skip] == ' ')
        {
            skip++;
        }

        // ��ȡ���������Ŀ¼�ļ� 
        m_request_file = (char*)malloc(uiLen);
        if (m_request_file)
        {
            int i=0;
            memset(m_request_file, 0, uiLen);
            while(m_linedata[skip+i] != ' ' && m_linedata[skip+i] != '\0')
            {
                m_request_file[i] = m_linedata[skip+i];
                i++;
            }

            // ������ȡ(��ת����,���õ�ʱ���ٴ���) 
            char* cmdstr = strchr(m_request_file, '?');
            if (cmdstr)
            {
                *cmdstr++ = '\0';
                m_request_command = cmdstr;
                while(*cmdstr != '\0')
                {
                    if (*cmdstr == '&')
                    {
                        *cmdstr = ' ';
                    }
                    cmdstr++;
                }
            }
        }
    }
    
    while(GetCompare(s))
    {
        if (stricmp(m_key, "Content-Length") == 0)
        {
            m_data_len = strtoul(m_value, NULL, 10);
        }
        else if (stricmp(m_key, "Accept-Language") == 0)
        {
            m_accept_language = strdup(m_value);
        }
        else if (stricmp(m_key, "Accept-Encoding") == 0)
        {
            m_accept_encoding = strdup(m_value);
        }
        else if (stricmp(m_key, "User-Agent") == 0)
        {
            m_user_agent = strdup(m_value);
        }
        else if (stricmp(m_key, "Cookie") == 0)
        {
            m_cookies = strdup(m_value);
        }
        else if (stricmp(m_key, "Accept") == 0)
        {
            m_accept = strdup(m_value);
        }
        else if (stricmp(m_key, "Content-Type") == 0)
        {
            m_content_type = strdup(m_value);
        }
        bret = true;
    }

    if (m_ispost && m_data_len!=0)
    {
        m_post_data = (char*)malloc(m_data_len+1);
        if (m_post_data)
        {
            recv(s, m_post_data, m_data_len, 0);
        }
    }
    return bret;
}