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

    m_request_path = NULL;
    m_request_command = NULL;

    m_client_socket = s;

    _InitializeListHead(&m_head_compare.next);

    AnalyzeRequestData();
}

RequestHeaderInfo::~RequestHeaderInfo()
{
    while(!_IsListEmpty(&m_head_compare.next))
    {
        PLIST_ENTRY li = _RemoveTailList(&m_head_compare.next);
        pkvp pvp = CONTAINING_RECORD(li, kvp, next);
        MFREE(pvp->key);
        MFREE(pvp->value)
        MFREE(pvp);
    }

    MFREE(m_linedata);
    MFREE(m_key);
    MFREE(m_value);
    MFREE(m_post_data);
    MFREE(m_request_path);
}

// ��ȡͷ�������һ������ 
unsigned int RequestHeaderInfo::GetLine()
{
    unsigned int i = 0;
    unsigned int uisize = START_SIZE;
    char *szstr = (char*)MALLC(uisize);
    memset(szstr, 0, uisize);

    MFREE(m_linedata);
    while(szstr)
    {
        while (i<uisize)
        {
            recv(m_client_socket, szstr+i, 1, 0);
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
        char* tmp = (char*)MALLC(uisize*2);
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
bool RequestHeaderInfo::GetCompare()
{
    unsigned int counts = GetLine();
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
    m_key = (char*)MALLC(counts+1);
    m_value = (char*)MALLC(counts+1);
    if (m_key && m_value)
    {
        memset(m_key, 0, counts+1);
        memset(m_value, 0, counts+1);
        memcpy(m_key, m_linedata, p-m_linedata);
        // �Թ��ո� 
        do{
            p++;
        }while (*p!='\0' && *p==' ');
        strcpy(m_value, p);
        return true;
    }

    MFREE(m_key);
    MFREE(m_value);
    return false;
}

// ͷ������ 
bool RequestHeaderInfo::AnalyzeMethod()
{
    // ��ȡ��һ�п��� GET ���� POST 
    unsigned int uiLen = GetLine(); 
    if (uiLen!=0)
    {
#ifdef _DEBUG
        SYSTEMTIME st;
        GetLocalTime(&st);
        printf("[%04u-%02u-%02u %02u:%02u:%02u] %s\n", 
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, 
            "==================================================================================");
        printf("[%04u-%02u-%02u %02u:%02u:%02u] %s\n", 
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, 
            m_linedata);
#endif

        if ( memicmp(m_linedata, "GET", 3) == 0 )
        {
            m_ispost = false;
        }
        else if (memicmp(m_linedata, "POST", 4) == 0)
        {
            m_ispost = true;
            m_data_len = 0;
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
        m_request_path = (char*)MALLC(uiLen);
        if (m_request_path)
        {
            int i=0;
            memset(m_request_path, 0, uiLen);
            // ��ȡ·�� 
            while(m_linedata[skip+i] != ' ' && m_linedata[skip+i] != '\0')
            {
                m_request_path[i] = m_linedata[skip+i];
                i++;
            }

            // ������ȡ(��ת����,���õ�ʱ���ٴ���) 
            char* cmdstr = strchr(m_request_path, '?');
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

        return true;
    }

    return false;
}

// ͷ�� 
void RequestHeaderInfo::AnalyzeHeadPair()
{
    while(GetCompare())
    {
        pkvp pvp = (pkvp)malloc(sizeof(kvp));
        if (pvp)
        {
#ifdef _DEBUG
            SYSTEMTIME st;
            GetLocalTime(&st);
            printf("[%04u-%02u-%02u %02u:%02u:%02u] %s: %s\n", 
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, 
                m_key, m_value);
#endif
            if (stricmp(m_key, "Content-Length") == 0)
            {
                m_data_len = strtoul(m_value, NULL, 10);
            }

            memset(pvp, 0, sizeof(kvp));
            pvp->key = strdup(m_key);
            pvp->value = strdup(m_value);
            _InsertTailList(&m_head_compare.next, &pvp->next);
        }
    }
}

// ��ȡͷ����ֵ 
const char* RequestHeaderInfo::GetHeader(const char* key)
{
    for (
            PLIST_ENTRY it = m_head_compare.next.Flink; 
            it != &m_head_compare.next; 
            it = it->Flink
        )
    {
        pkvp pvp = CONTAINING_RECORD(it, kvp, next);
        if (stricmp(pvp->key, key) == 0)
        {
            return pvp->value;
        }
    }
    return "";
}

// ͷ�����󸽼����� 
unsigned long RequestHeaderInfo::AnalyzeHeadContent()
{
    if (m_data_len!=0)
    {
        MFREE(m_post_data);
        m_post_data = (char*)MALLC(m_data_len+1);
        if (m_post_data)
        {
            unsigned long l = 0;
            while(l < m_data_len)
            {
                int n = recv(m_client_socket, m_post_data+l, m_data_len-l, 0);
                if (n == SOCKET_ERROR)
                {
                    break;
                }
                l += n;
            }
#ifdef _DEBUG
            if (l == m_data_len)
            {
                printf("err: read Content Data(%u/%u)\n", l, m_data_len);
            }
#endif

            return l;
        }
    }

    return 0;
}

// ������������ 
bool RequestHeaderInfo::AnalyzeRequestData()
{
    bool bret = false;
    
    if (AnalyzeMethod())
    {
        // ��ֵ������ 
        AnalyzeHeadPair();
        // ����и�������,ȡ���� 
        AnalyzeHeadContent();
        bret = true;
    }
    return bret;
}