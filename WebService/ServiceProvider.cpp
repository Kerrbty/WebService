#include "ServiceProvider.h"
#include "common.h"
#include <Shlwapi.h>
#include <stdio.h>
#pragma comment(lib, "shlwapi")

#define WEB_DIR     "www"
#define CGI_DIR     "www\\cgi"
#define CGI_START_SIZE  4096

static char* g_web_root_path = NULL;

// 根据GET/POST请求 将URL路径转成本机的文件路径 
static bool ChgLocalPath(const char* szPathFile, const char* szDirName, char* szOutPath, unsigned int dwlen)
{
    bool bret = false;
    if (szPathFile == NULL || szOutPath == NULL || dwlen == 0)
    {
        return false;
    }

    if (g_web_root_path == NULL)
    {
        g_web_root_path = (char*)malloc(MAX_PATH*2);
        if (g_web_root_path)
        {
            GetModuleFileNameA(GetModuleHandle(NULL), g_web_root_path, MAX_PATH);
            PathRemoveFileSpecA(g_web_root_path);
            strcat(g_web_root_path, "\\");
            printf("local web dir: %s%s\n", g_web_root_path, WEB_DIR);
        }
    }

    if (g_web_root_path == NULL)
    {
        return false;
    }

    // 拼凑完整路径 
    memset(szOutPath, 0, dwlen);
    strncpy(szOutPath, g_web_root_path, dwlen);
    strncat(szOutPath, szDirName, dwlen);
    int len = strlen(szOutPath);
    int i=0;
    while(len < dwlen)
    {
        if (szPathFile[i] == '\0' || 
            szPathFile[i] == '?' || 
            szPathFile[i] == '&' || 
            szPathFile[i] == '%')
        {
            szOutPath[len] = '\0';
            break;
        }
        else if (szPathFile[i] == '/')
        {
            szOutPath[len++] = '\\';
        }
        else
        {
            szOutPath[len++] = szPathFile[i];
        }
        i++;
    }
    return true;
}

// 查询是否是本地静态文件 
static bool IsRequestStaticFile(const char* szPathFile)
{
    bool bret = false;
    if (szPathFile == NULL)
    {
        return bret;
    }

    unsigned int needlen = strlen(szPathFile)+MAX_PATH*2;
    char* fullpath = (char*)malloc(needlen);
    if( ChgLocalPath(szPathFile, WEB_DIR, fullpath, needlen) )
    {
        // 读取文件 
        HANDLE hFile = CreateFileA(fullpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile!= INVALID_HANDLE_VALUE)
        {
            bret = true;
            CloseHandle(hFile);
        }
    }
    MFREE(fullpath);
    return bret;
}

// 查询是否是CGI请求 
static bool IsRequestCGIFile(const char* szPathFile)
{
    bool bret = false;
    if (szPathFile == NULL)
    {
        return bret;
    }
    unsigned int needlen = strlen(szPathFile)+MAX_PATH*2;
    char* fullpath = (char*)malloc(needlen);
    if( ChgLocalPath(szPathFile, CGI_DIR, fullpath, needlen) )
    {
#ifdef _DEBUG
        strncat(fullpath, "_d.cgi", needlen);
#else
        strncat(fullpath, ".cgi", needlen);
#endif
        // 读取文件 
        HANDLE hFile = CreateFileA(fullpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile!= INVALID_HANDLE_VALUE)
        {
            bret = true;
            CloseHandle(hFile);
        }
    }
    MFREE(fullpath);
    return bret;
}

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

    unsigned int needlen = strlen(szPathFile)+MAX_PATH*2;
    char* fullpath = (char*)malloc(needlen);
    if( ChgLocalPath(szPathFile, WEB_DIR, fullpath, needlen) )
    {
        const char* fileext = PathFindExtensionA(fullpath);
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
                    // 具体扩充 访问 https://www.runoob.com/http/http-content-type.html 
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
                    else if (stricmp(fileext, ".ico") == 0)
                    {
                        sztype = "Content-Type: image/x-icon\r\n";
                    }
                }
            }
            send(s, sztype, strlen(sztype), 0);
            bret = true;
        }
    }
    MFREE(fullpath);
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

    // 4k 一个块发送 
    const DWORD dwBlockSize = 4*1024;
    char* buffer = (char*)malloc(dwBlockSize);
    if (buffer == NULL)
    {
        return false;
    }

    unsigned int needlen = strlen(szPathFile)+MAX_PATH*2;
    char* fullpath = (char*)malloc(needlen);
    do 
    {
        if( !ChgLocalPath(szPathFile, WEB_DIR, fullpath, needlen) )
        {
            break;
        }

        // 读取文件 
        HANDLE hFile = CreateFileA(fullpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile!= INVALID_HANDLE_VALUE)
        {
            // 发送头部长度 
            DWORD dwFileSize = GetFileSize(hFile, NULL);
            wsprintfA(buffer, "Content-Length: %u\r\n\r\n", dwFileSize);
            send(s, buffer, strlen(buffer), 0);

            // 发送文件 
            DWORD dwBytes = 0;
            SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
            while( ReadFile(hFile, buffer, dwBlockSize, &dwBytes, NULL) && dwBytes>0)
            {
                send(s, buffer, dwBytes, 0);
            }
            CloseHandle(hFile);
            bret = true;
        }
    } while (false);
    MFREE(fullpath);
    MFREE(buffer);
    return bret;
}

// 执行CGI并发送结果 
static bool DoSendCGIResult(SOCKET s, const char* szPathFile, const char* szCommandLine)
{
    bool bret = false;
    if (szPathFile == NULL)
    {
        return bret;
    }

    unsigned int needlen = strlen(szPathFile)+MAX_PATH*2;
    char* fullpath = (char*)malloc(needlen+2);
    fullpath[0] = '"';
    if( ChgLocalPath(szPathFile, CGI_DIR, fullpath+1, needlen) )
    {
#ifdef _DEBUG
        strncat(fullpath, "_d.cgi\"", needlen);
#else
        strncat(fullpath, ".cgi\"", needlen);
#endif
        FILE* fp = NULL;
        if (szCommandLine)
        {
            char* NewRunCmd = (char*)malloc(strlen(fullpath)+strlen(szCommandLine)+8);
            if (NewRunCmd)
            {
                sprintf(NewRunCmd, "%s %s", fullpath, szCommandLine);
                fp = _popen(NewRunCmd, "rt");
                MFREE(NewRunCmd);
            }
        }
        if(fp == NULL) 
        {
            fp = _popen(fullpath, "rt");
        }
        if (fp)
        {
            unsigned int uisize = CGI_START_SIZE;
            char *szstr = (char*)malloc(uisize);
            memset(szstr, 0, uisize);
            while( !feof( fp ) )
            {
                if (fgets(szstr, uisize, fp))
                {
                    // 头部数据 
                    if ( strcmp(szstr, "\r\n") == 0 )
                    {
                        // 这时候就需要发送后面长度了 
                        break; 
                    }
                    else
                    {
                        send(s, szstr, strlen(szstr), 0);
                    }
                }
            }

            // 需要接收所有数据 
            unsigned int ipos = 0;
            memset(szstr, 0, uisize);
            char ch;
            while( fread(&ch, 1, 1, fp) )
            {
                if (ipos>=uisize)
                {
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
                if (szstr)
                {
                    szstr[ipos++] = ch;
                }
            }

            if (szstr)
            {
                // 发送头部长度 
                unsigned int bodylen = strlen(szstr);
                char* contentlen = (char*)malloc(64);
                if (contentlen)
                {
                    wsprintfA(contentlen, "Content-Length: %u\r\n\r\n", bodylen);
                    send(s, contentlen, strlen(contentlen), 0);
                }
                // 发送正文 
                send(s, szstr, bodylen, 0);
            }
        }

        // 读取文件 
        HANDLE hFile = CreateFileA(fullpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile!= INVALID_HANDLE_VALUE)
        {
            bret = true;
            CloseHandle(hFile);
        }
    }
    MFREE(fullpath);
    return bret;
}

// 服务器完成请求 
bool ResponseData(SOCKET s, RequestHeaderInfo *request)
{
    // 主页 
    const char* requestfile = request->GetRequestFile();
    if (requestfile == NULL)
    {
        return false;
    }
    if (strcmp(requestfile, "/") == 0)
    {
        if (IsRequestStaticFile("/index.htm"))
        {
            requestfile = "/index.htm";
        }
        else if (IsRequestStaticFile("/index.html"))
        {
            requestfile = "/index.html";
        }
    }

    // 获取URL编码并转出成Windows的路径 
    unsigned int len = strlen(requestfile);
    char* newFile = (char*)malloc(len+MAX_PATH);
    if (newFile == NULL)
    {
        return false;
    }
    char* newrequestfile = UrlDecode(requestfile, len, newFile, len+MAX_PATH);

    if (IsRequestStaticFile(newrequestfile))
    {
        // 成功信息 
        const char* http_ok = "HTTP/1.1 200 OK\r\n";
        send(s, http_ok, strlen(http_ok), 0);

        // 服务器信息 
        SendServiceInfo(s);

        // 发送文件类型 
        SendFileType(s, newrequestfile);

        // 发送文件 
        SendPage(s, newrequestfile);
    }
    else if (IsRequestCGIFile(newrequestfile))
    {
        // 成功信息 
        const char* http_ok = "HTTP/1.1 200 OK\r\n";
        send(s, http_ok, strlen(http_ok), 0);

        // 发送文件类型 
        SendServiceInfo(s);

        // 执行CGI并返回数据 
        DoSendCGIResult(s, newrequestfile, request->GetCommandLine());
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
    MFREE(newFile);
    return true;
}