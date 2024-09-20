#include "ServiceProvider.h"
#include "Base64/Base64.h"
#include "common.h"
#include <Shlwapi.h>
#include <stdio.h>
#include <time.h>
#pragma comment(lib, "shlwapi")

#define WEBSOCKET_GUID   "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
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
            strcat_s(g_web_root_path, MAX_PATH, "\\");
            printf("local web dir: %s%s\n", g_web_root_path, WEB_DIR);
        }
    }

    if (g_web_root_path == NULL)
    {
        return false;
    }

    // 拼凑完整路径 
    memset(szOutPath, 0, dwlen);
    strcpy_s(szOutPath, dwlen, g_web_root_path);
    strcat_s(szOutPath, dwlen, szDirName);
    unsigned int len = strlen(szOutPath);
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
        strcat_s(fullpath, needlen, "_d.cgi");
#else
        strcat_s(fullpath, needlen, ".cgi");
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

// 服务端程序信息,当前时间  
static void SendServiceInfo(SOCKET s, bool isWS = false)
{
    // 服务器工具信息 
    const char* service_info = "Server: WebSercice/1.0\r\n";
    if (isWS)
    {
        service_info = "Server: WebSercice/1.1 websockets/1.0.0\r\n";
    }
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
}

// 服务端信息时间,位置，长链接 
static void SendWebService(SOCKET s)
{
    SendServiceInfo(s);

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
                    if (_stricmp(fileext, ".htm") == 0 || _stricmp(fileext, ".html") == 0 )
                    {
                        sztype = "Content-Type: text/html;charset=utf-8\r\n";
                    }
                    else if (_stricmp(fileext, ".txt") == 0)
                    {
                        sztype = "Content-Type: text/plain\r\n";
                    }
                    else if (_stricmp(fileext, ".js") == 0)
                    {
                        sztype = "Content-Type: application/javascript\r\n";
                    }
                    else if (_stricmp(fileext, ".css") == 0)
                    {
                        sztype = "Content-Type: text/css\r\n";
                    }
                    else if (_stricmp(fileext, ".png") == 0)
                    {
                        sztype = "Content-Type: image/png\r\n";
                    }
                    else if (_stricmp(fileext, ".jpg") == 0)
                    {
                        sztype = "Content-Type: image/jpeg\r\n";
                    }
                    else if (_stricmp(fileext, ".gif") == 0)
                    {
                        sztype = "Content-Type: image/gif\r\n";
                    }
                    else if (_stricmp(fileext, ".ico") == 0)
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
        strcat_s(fullpath, needlen, "_d.cgi\"");
#else
        strcat_s(fullpath, needlen, ".cgi\"");
#endif
        FILE* fp = NULL;
        if (szCommandLine)
        {
            char* NewRunCmd = (char*)malloc(strlen(fullpath)+strlen(szCommandLine)+8);
            if (NewRunCmd)
            {
                wsprintfA(NewRunCmd, "%s %s", fullpath, szCommandLine);
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

// websocket 处理 
static bool UpgradeSocket(SOCKET s, RequestHeaderInfo *request)
{
    const char* pWebSocketKey = request->GetHeader("Sec-WebSocket-Key");
    if (pWebSocketKey == NULL)
    {
        return false;
    }

    int len = strlen(pWebSocketKey);
    len += sizeof(WEBSOCKET_GUID) + 2;
    char* serial_websocket_key = (char*)malloc(len);
    if (serial_websocket_key)
    {
        const char* response_code = "HTTP/1.1 101 Switching Protocols\r\n";
        send(s, response_code, strlen(response_code), 0);
        
        const char* upgrade = "Upgrade: websocket\r\nConnection: Upgrade\r\n";
        send(s, upgrade, strlen(upgrade), 0);

        const char* sec_key = "Sec-WebSocket-Accept: ";
        send(s, sec_key, strlen(sec_key), 0);
        unsigned char data[24] = {0};
        strcpy_s(serial_websocket_key, len, pWebSocketKey);
        strcat_s(serial_websocket_key, len, WEBSOCKET_GUID);
        calcBufferSha1((unsigned char*)serial_websocket_key, strlen(serial_websocket_key), data);
        memset(serial_websocket_key, 0, len);
        int outlen = Base64_encode((char*)data, 20, serial_websocket_key, len);
        send(s, serial_websocket_key, outlen, 0);
        send(s, "\r\n", 2, 0);

        SendServiceInfo(s, true);
        send(s, "\r\n", 2, 0);
    }
    return true;
}

// websocket 各种处理 
char* recv_websocket(SOCKET s)
{
    // 标志位 FIN = 0x80 | Opcode = 0x01 
    unsigned char ch = '\0';
    if (recv(s, (char*)&ch, 1, 0) < 0)
    {
        printf("拿不到数据\n");
        return NULL;
    }
    // 0x81 Fin | Text
    // 0x88 Fin | Close 
    if (ch != 0x81)
    {
        // 不是文本全跳过 
        return NULL;
    }

    bool bMask = false;
    // 长度 
    unsigned int mask = 0;
    unsigned long long payload_len = 0;
    if (recv(s, (char*)&ch, 1, 0) < 0)
    {
        return NULL;
    }

    payload_len = (unsigned char)(ch&0x7F);
    bMask = ((ch&0x80) == 0x80);
    if ( payload_len == 0x7E )
    {
        unsigned short v = 0;
        recv(s, (char*)&v, 2, 0);
        payload_len = ntohs(v);
    }
    else if ( payload_len == 0x7F )
    {
        LARGE_INTEGER la1;
        LARGE_INTEGER la2;
        unsigned long long m = 0;
        recv(s, (char*)&m, 8, 0);
        la1.QuadPart = m;
        la2.LowPart = ntohl(la1.HighPart);
        la2.HighPart = ntohl(la1.LowPart);
        payload_len = la2.QuadPart;
    }

    // mask 
    if (bMask)
    {
        recv(s, (char*)&mask, 4, 0);
    }

    // 读数据 
    char* data = (char*)malloc(payload_len + 2);
    if (data)
    {
        unsigned long long l = 0;
        memset(data, 0, payload_len+2);
        while(l < payload_len)
        {
            int n = recv(s, data+l, payload_len-l, 0);
            if (n < 0)
            {
                break;
            }
            l += n;
        }

        // 解码 
        if (bMask)
        {
            for (unsigned long long i=0; i<payload_len; i++)
            {
                data[i] ^= ((mask>>((i%4)*8))&0xFF);
            }
        }
    }
    return data;
}
void send_websocket(SOCKET s, char* data, unsigned long len)
{
    int buf_len = len;
    bool bMask = false;

    if (len < 126)
    {
        buf_len += 1 + 1;
    }
    else if (len <= 0xFFFF)
    {
        buf_len += 1 + 1 + 2;
    }
    else
    {
        buf_len += 1 + 1 + 8;
    }
    if (bMask)
    {
        buf_len += 4;
    }

    unsigned char* buf = (unsigned char*)malloc(buf_len);
    if (buf == NULL)
    {
        return;
    }

    // flags 
    buf[0] = 0x81;
    // len 
    if (len < 126)
    {
        // + mask 
        buf[1] = len;
        
    }
    else if ( len <= 0xFFFF )
    {
        buf[1] = 0x7E;
        unsigned short blen = ntohs(len);
        memcpy(buf+2, &blen, 2);
    }
    else
    {
        LARGE_INTEGER la;
        buf[1] = 0x7F;
        la.HighPart = ntohl(len&0xFFFFFFFF);
        la.LowPart = ntohl((len>>32)&0xFFFFFFFF);
        memcpy(buf+2, (char*)&la.QuadPart, 8);
    }

    // mask 
    if (bMask)
    {
        buf[1] |= 0x80;

        unsigned int mask = ((rand()%0xFE + 1)<<24) | ((rand()%0xFE + 1)<<16) | ((rand()%0xFE + 1)<<8) | (rand()%0xFE + 1);
        memcpy(buf+buf_len-len-4, (char*)&mask, 4);
        for (unsigned long i=0; i<len; i++)
        {
            buf[buf_len-len+i] = data[i] ^ ((mask>>((i%4)*8))&0xFF);
        }
    }
    else
    {
        memcpy(buf+buf_len-len, data, len);
    }

    send(s, (char*)buf, buf_len, 0);
    Sleep(1);
}
void close_websocket(SOCKET s)
{
    // 关闭websocket 
    send(s, "\x88\x02\x03\xe8", 4, 0);
}

// 处理 websocket 通讯 
void MonitorWebSocket(SOCKET s)
{
    srand((unsigned int)time(NULL));
    while(TRUE)
    {
        char* data = recv_websocket(s);
        if (data == NULL)
        {
            break;
        }
        printf("%s\n", data);
        MFREE(data);

        send_websocket(s, "hello world!", 12);
    }
    close_websocket(s);    
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

    const char* pupgrade = request->GetHeader("Connection");
    if ( pupgrade && _stricmp(pupgrade, "Upgrade")==0 )
    {
        if ( UpgradeSocket(s, request) )
        {
            MonitorWebSocket(s);
        }
    }
    else if (IsRequestStaticFile(newrequestfile))
    {
        // 成功信息 
        const char* http_ok = "HTTP/1.1 200 OK\r\n";
        send(s, http_ok, strlen(http_ok), 0);

        // 服务器信息 
        SendWebService(s);

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
        SendWebService(s);

        // 执行CGI并返回数据 
        DoSendCGIResult(s, newrequestfile, request->GetCommandLine());
    }
    else 
    {
        // 没找到,返回404页面 
        const char* http_404 = "HTTP/1.1 404 Not Found\r\n";
        send(s, http_404, strlen(http_404), 0);

        // 服务器信息 
        SendWebService(s);

        // 发送文本类型 
        SendFileType(s, "/404.htm");

        // 发送html 
        SendPage(s, "/404.htm");
    }
    MFREE(newFile);
    return true;
}