#include "ServiceProvider.h"
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi")

static char* g_web_root_path = NULL;

// ��ѯ�Ƿ��Ǳ��ؾ�̬�ļ� 
static bool IsRequestStaticFile(const char* szPathFile)
{
    bool bret = false;
    if (szPathFile == NULL)
    {
        return bret;
    }

    if (g_web_root_path == NULL)
    {
        g_web_root_path = (char*)malloc(MAX_PATH);
        if (g_web_root_path)
        {
            GetModuleFileNameA(GetModuleHandle(NULL), g_web_root_path, MAX_PATH);
            PathRemoveFileSpecA(g_web_root_path);
            strcat(g_web_root_path, "\\www");
        }
    }

    if (g_web_root_path == NULL)
    {
        return false;
    }

    char* fullpath = (char*)malloc(strlen(szPathFile)+strlen(g_web_root_path)+MAX_PATH);
    if (fullpath)
    {
        // ƴ������·�� 
        strcpy(fullpath, g_web_root_path);
        int len = strlen(fullpath);
        fullpath[len] = '\0';
        int i=0;
        while(true)
        {
            if (szPathFile[i] == '\0' || 
                szPathFile[i] == '?' || 
                szPathFile[i] == '&' || 
                szPathFile[i] == '%')
            {
                fullpath[len] = '\0';
                break;
            }
            else if (szPathFile[i] == '/')
            {
                fullpath[len++] = '\\';
            }
            else
            {
                fullpath[len++] = szPathFile[i];
            }
            i++;
        }

        // ��ȡ�ļ� 
        HANDLE hFile = CreateFileA(fullpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile!= INVALID_HANDLE_VALUE)
        {
            bret = true;
            CloseHandle(hFile);
        }
        MFREE(fullpath);
    }
    return bret;
}

// ��ѯ�Ƿ���CGI���� 
static bool IsRequestCGIFile(const char* szPathFile)
{
    if (szPathFile == NULL)
    {
        return false;
    }
    return false;
}


//////////////////////////////////////////////////////////////////////////
// ��ʼ���͵�ͷ����Ϣ 


// ����
static const char* Days[] = {
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "The",
    "Fri",
    "Sat",
};

// �·� 
static const char* Mons[] = {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jue",
    "Jul",
    "Aug",
    "Spet",
    "Oct",
    "Nov",
    "Dec",
};

// �������̶�����Ϣ 
static void SendServiceInfo(SOCKET s)
{
    // ������������Ϣ 
    const char* service_info = "Server: WebSercice/1.0\r\n";
    send(s, service_info, strlen(service_info), 0);

    // Ӧ��ʱ�� 
    char* data = (char*)malloc(1024);
    if (data)
    {
        SYSTEMTIME systime;
        GetSystemTime(&systime);
        int len = wsprintfA(data, "Date: %s, %02u %s %u %02u:%02u:%02u GMT\r\n", 
            Days[systime.wDayOfWeek], systime.wDay, Mons[systime.wMonth-1], systime.wYear,
            systime.wHour, systime.wMinute, systime.wSecond);
        send(s, data, len, 0);
        MFREE(data);
    }

    // ������ 
    const char* keep_alive = "Connection: Keep-Alive\r\n";
    send(s, keep_alive, strlen(keep_alive), 0);

    // ���ص�ַ 
    const char* local_info = "Location: http://localhost/\r\n";
    send(s, local_info, strlen(local_info), 0);
}

// �жϲ������ļ����� 
static bool SendFileType(SOCKET s, const char* szPathFile)
{
    bool bret = false;
    if (szPathFile == NULL)
    {
        return bret;
    }

    if (g_web_root_path == NULL)
    {
        g_web_root_path = (char*)malloc(MAX_PATH);
        if (g_web_root_path)
        {
            GetModuleFileNameA(GetModuleHandle(NULL), g_web_root_path, MAX_PATH);
            PathRemoveFileSpecA(g_web_root_path);
            strcat(g_web_root_path, "\\www");
        }
    }
    if (g_web_root_path == NULL)
    {
        return bret;
    }

    char* fullpath = (char*)malloc(strlen(szPathFile)+strlen(g_web_root_path)+MAX_PATH);
    if (fullpath)
    {
        // ƴ������·�� 
        strcpy(fullpath, g_web_root_path);
        int len = strlen(fullpath);
        fullpath[len] = '\0';
        int i=0;
        while(true)
        {
            if (szPathFile[i] == '\0' || 
                szPathFile[i] == '?' || 
                szPathFile[i] == '&' || 
                szPathFile[i] == '%')
            {
                fullpath[len] = '\0';
                break;
            }
            else if (szPathFile[i] == '/')
            {
                fullpath[len++] = '\\';
            }
            else
            {
                fullpath[len++] = szPathFile[i];
            }
            i++;
        }

        const char* fileext = PathFindExtensionA(szPathFile);

        // ��ȡ�ļ� 
        HANDLE hFile = CreateFileA(fullpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile!= INVALID_HANDLE_VALUE)
        {
            // ����ͷ������ 
            DWORD dwBytes = 0;
            unsigned char buffer[32] = {0};
            SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
            ReadFile(hFile, buffer, 32, &dwBytes, NULL);
            CloseHandle(hFile);

            const char* sztype = "Content-Type: text/html\r\n";
            if (memcmp(buffer, "\x89\x50\x4E\x47", 4) == 0)
            {
                sztype = "Content-Type: image/png\r\n";
            }
            else if (memcmp(buffer, "\xFF\xD8\xFF\xE0\x00\x10\x4A\x46\x49\x46", 10) == 0)
            {
                sztype = "Content-Type: image/jpeg\r\n";
            }
            else if (memcmp(buffer, "GIF8", 4) == 0)
            {
                sztype = "Content-Type: image/gif\r\n";
            }
            else 
            {
                if (fileext)
                {
                    if (stricmp(fileext, ".htm") == 0 || stricmp(fileext, ".html") == 0 )
                    {
                        sztype = "Content-Type: text/html;charset=utf-8\r\n";
                    }
                    else if (stricmp(fileext, ".txt") == 0)
                    {
                        sztype = "Content-Type: text/plain\r\n";
                    }
                    else if (stricmp(fileext, ".js") == 0)
                    {
                        sztype = "Content-Type: application/javascript\r\n";
                    }
                    else if (stricmp(fileext, ".css") == 0)
                    {
                        sztype = "Content-Type: text/css\r\n";
                    }
                    else if (stricmp(fileext, ".png") == 0)
                    {
                        sztype = "Content-Type: image/png\r\n";
                    }
                    else if (stricmp(fileext, ".jpg") == 0)
                    {
                        sztype = "Content-Type: image/jpeg\r\n";
                    }
                    else if (stricmp(fileext, ".gif") == 0)
                    {
                        sztype = "Content-Type: image/gif\r\n";
                    }
                }
            }
            send(s, sztype, strlen(sztype), 0);
        }
        MFREE(fullpath);
    }
    return bret;
}

// �����ļ� 
static bool SendPage(SOCKET s, const char* szPathFile)
{
    bool bret = false;
    if (szPathFile == NULL)
    {
        return bret;
    }

    if (g_web_root_path == NULL)
    {
        g_web_root_path = (char*)malloc(MAX_PATH);
        if (g_web_root_path)
        {
            GetModuleFileNameA(GetModuleHandle(NULL), g_web_root_path, MAX_PATH);
            PathRemoveFileSpecA(g_web_root_path);
            strcat(g_web_root_path, "\\www");
        }
    }
    if (g_web_root_path == NULL)
    {
        return bret;
    }

    // 4k һ���鷢�� 
    DWORD dwBlockSize = 4*1024;
    char* buffer = (char*)malloc(dwBlockSize);
    if (buffer == NULL)
    {
        return false;
    }

    char* fullpath = (char*)malloc(strlen(szPathFile)+strlen(g_web_root_path)+MAX_PATH);
    if (fullpath)
    {
        // ƴ������·�� 
        strcpy(fullpath, g_web_root_path);
        int len = strlen(fullpath);
        fullpath[len] = '\0';
        int i=0;
        while(true)
        {
            if (szPathFile[i] == '\0' || 
                szPathFile[i] == '?' || 
                szPathFile[i] == '&' || 
                szPathFile[i] == '%')
            {
                fullpath[len] = '\0';
                break;
            }
            else if (szPathFile[i] == '/')
            {
                fullpath[len++] = '\\';
            }
            else
            {
                fullpath[len++] = szPathFile[i];
            }
            i++;
        }

        // ��ȡ�ļ� 
        HANDLE hFile = CreateFileA(fullpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile!= INVALID_HANDLE_VALUE)
        {
            // ����ͷ������ 
            DWORD dwFileSize = GetFileSize(hFile, NULL);
            wsprintfA(buffer, "Content-Length: %ul\r\n\r\n", dwFileSize);
            send(s, buffer, strlen(buffer), 0);

            // �����ļ� 
            DWORD dwBytes = 0;
            SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
            while( ReadFile(hFile, buffer, dwBlockSize, &dwBytes, NULL) && dwBytes>0)
            {
                send(s, buffer, dwBytes, 0);
            }
            CloseHandle(hFile);
        }
        MFREE(fullpath);
    }
    MFREE(buffer);
    return bret;
}

// ������������� 
bool ResponseData(SOCKET s, RequestHeaderInfo *request)
{
    // ��ҳ 
    const char* requestfile = request->GetRequestFile();
    if (requestfile && strcmp(requestfile, "/") == 0)
    {
        requestfile = "/index.html";
    }

    if (IsRequestStaticFile(requestfile))
    {
        // �ɹ���Ϣ 
        const char* http_ok = "HTTP/1.1 200 OK\r\n";
        send(s, http_ok, strlen(http_ok), 0);

        // ��������Ϣ 
        SendServiceInfo(s);

        // �����ļ����� 
        SendFileType(s, requestfile);

        // �����ļ� 
        SendPage(s, requestfile);
    }
    else if (IsRequestCGIFile(requestfile))
    {
        // �ɹ���Ϣ 
        const char* http_ok = "HTTP/1.1 200 OK\r\n";
        send(s, http_ok, strlen(http_ok), 0);

        // �����ļ����� 
        SendServiceInfo(s);

        // ִ��CGI���������� 
        ;
    }
    else 
    {
        // û�ҵ�,����404ҳ�� 
        const char* http_404 = "HTTP/1.1 404 Not Found\r\n";
        send(s, http_404, strlen(http_404), 0);

        // ��������Ϣ 
        SendServiceInfo(s);

        // �����ı����� 
        SendFileType(s, "/404.htm");

        // ����html 
        SendPage(s, "/404.htm");
    }
    return true;
}