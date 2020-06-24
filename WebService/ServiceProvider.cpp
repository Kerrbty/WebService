#include "ServiceProvider.h"
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi")

static char* g_web_root_path = NULL;

// 查询是否是本地静态文件 
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
        // 拼凑完整路径 
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

        // 读取文件 
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

// 查询是否是CGI请求 
static bool IsRequestCGIFile(const char* szPathFile)
{
    if (szPathFile == NULL)
    {
        return false;
    }
    return false;
}


//////////////////////////////////////////////////////////////////////////
// 开始发送的头部信息 


// 星期
static const char* Days[] = {
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "The",
    "Fri",
    "Sat",
};

// 月份 
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

// 服务器固定的信息 
static void SendServiceInfo(SOCKET s)
{
    // 服务器工具信息 
    const char* service_info = "Server: WebSercice/1.0\r\n";
    send(s, service_info, strlen(service_info), 0);

    // 应答时间 
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

    // 长连接 
    const char* keep_alive = "Connection: Keep-Alive\r\n";
    send(s, keep_alive, strlen(keep_alive), 0);

    // 本地地址 
    const char* local_info = "Location: http://localhost/\r\n";
    send(s, local_info, strlen(local_info), 0);
}

// 判断并发送文件类型 
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
        // 拼凑完整路径 
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

        // 读取文件 
        HANDLE hFile = CreateFileA(fullpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile!= INVALID_HANDLE_VALUE)
        {
            // 发送头部长度 
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

// 发送文件 
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

    // 4k 一个块发送 
    DWORD dwBlockSize = 4*1024;
    char* buffer = (char*)malloc(dwBlockSize);
    if (buffer == NULL)
    {
        return false;
    }

    char* fullpath = (char*)malloc(strlen(szPathFile)+strlen(g_web_root_path)+MAX_PATH);
    if (fullpath)
    {
        // 拼凑完整路径 
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

        // 读取文件 
        HANDLE hFile = CreateFileA(fullpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile!= INVALID_HANDLE_VALUE)
        {
            // 发送头部长度 
            DWORD dwFileSize = GetFileSize(hFile, NULL);
            wsprintfA(buffer, "Content-Length: %ul\r\n\r\n", dwFileSize);
            send(s, buffer, strlen(buffer), 0);

            // 发送文件 
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

// 服务器完成请求 
bool ResponseData(SOCKET s, RequestHeaderInfo *request)
{
    // 主页 
    const char* requestfile = request->GetRequestFile();
    if (requestfile && strcmp(requestfile, "/") == 0)
    {
        requestfile = "/index.html";
    }

    if (IsRequestStaticFile(requestfile))
    {
        // 成功信息 
        const char* http_ok = "HTTP/1.1 200 OK\r\n";
        send(s, http_ok, strlen(http_ok), 0);

        // 服务器信息 
        SendServiceInfo(s);

        // 发送文件类型 
        SendFileType(s, requestfile);

        // 发送文件 
        SendPage(s, requestfile);
    }
    else if (IsRequestCGIFile(requestfile))
    {
        // 成功信息 
        const char* http_ok = "HTTP/1.1 200 OK\r\n";
        send(s, http_ok, strlen(http_ok), 0);

        // 发送文件类型 
        SendServiceInfo(s);

        // 执行CGI并返回数据 
        ;
    }
    else 
    {
        // 没找到,返回404页面 
        const char* http_404 = "HTTP/1.1 404 Not Found\r\n";
        send(s, http_404, strlen(http_404), 0);

        // 服务器信息 
        SendServiceInfo(s);

        // 发送文本类型 
        SendFileType(s, "/404.htm");

        // 发送html 
        SendPage(s, "/404.htm");
    }
    return true;
}