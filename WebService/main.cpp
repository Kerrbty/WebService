// 入口及socket连接过程 

#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include "AnalyzeRequestHttp.h"
#include "ServiceProvider.h"
#include "common.h"
#include <Windows.h>
#include <tchar.h>

#pragma comment(lib, "ws2_32.lib")

// http协议服务 
static DWORD CALLBACK DoService(LPVOID lpThreadParameter)
{
    DWORD dwsize = 4096;
    SOCKET s = (SOCKET)lpThreadParameter;
    char* buf=(char*)malloc(dwsize);

    // 接收请求,解析头信息 
    ZeroMemory(buf, dwsize);
    RequestHeaderInfo request(s);
//     printf("%s\n", request.GetUserAgent());

    // 调用对应的资源文件,回复信息 
    ResponseData(s, &request);

    // 关闭连接 
    closesocket(s);
    free(buf);
    return 0;
}

// 监听80端口进行服务 
static INT WebService()
{
    INT bRetVal = 1;
    WSADATA WsData; 
    WORD wVersionRequested = MAKEWORD(1, 1); 
    WSAStartup(wVersionRequested, &WsData);
    do 
    {
        SOCKET ss = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if ( INVALID_SOCKET == ss )
        {
            printf("socket call error: %d\n", WSAGetLastError());
            break;
        }
        else
        {
            do 
            {
                // 绑定本机80端口 
                struct sockaddr_in addr = {0};
                addr.sin_family = AF_INET;
                addr.sin_port = ntohs(80);
                addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
                if (
                    SOCKET_ERROR == 
                    bind(ss, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) 
                    )
                {
                    printf("bind call error: %d\n", WSAGetLastError());
                    break;
                }

                // 监听同时可服务20个连接 
                if (SOCKET_ERROR == listen(ss, 20))
                {
                    printf("listen call error: %d\n", WSAGetLastError());
                    break;
                }

                bRetVal = 0;
                struct sockaddr_in so = {0};
                while(TRUE)
                {
                    // 成功连接客户单独开辟线程处理服务 
                    memset(&so, 0, sizeof(so));
                    int lenth = sizeof(so);
                    SOCKET se = accept(ss, (struct sockaddr FAR *)&so, &lenth);
                    if (se != SOCKET_ERROR)
                    {
                        CloseHandle(CreateThread(NULL, 0, DoService, (LPVOID)(se), 0, NULL));
                    }
                }
            } while (FALSE);
            closesocket(ss);
        }
    } while (FALSE);
    WSACleanup();
    return bRetVal;
}

// 运行进程 
static HANDLE RunProcess(LPTSTR lpCmd)
{
    STARTUPINFO si = {sizeof(STARTUPINFO)};  
    PROCESS_INFORMATION pi;  

    si.wShowWindow = SW_SHOW; 
    si.dwFlags = STARTF_USESHOWWINDOW; 
    if(CreateProcess(
        NULL, 
        lpCmd, 
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi))  
    {  
        CloseHandle(pi.hThread);
        return pi.hProcess;
    } 
    return NULL;
}

// 守护进程 
static void DaemonProcess(DWORD pid, LPTSTR lpcmdline)
{
    if (pid != 0)
    {
        // 已有进程等待 
        HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);
        WaitForSingleObject(hProcess, INFINITE);
        CloseHandle(hProcess);
    }

    // 启动进程并等待 
    LPTSTR lpCmd = (LPTSTR)malloc(MAX_PATH*2*sizeof(TCHAR));
    if (lpCmd == NULL)
    {
        return;
    }

    lpCmd[0] = TEXT('"');
    GetModuleFileName(GetModuleHandle(NULL), lpCmd+1, MAX_PATH);
    int len = _tcslen(lpCmd);
    wsprintf(lpCmd+len, lpcmdline, GetCurrentProcessId());
    while(TRUE)
    {
        HANDLE hProcess = RunProcess(lpCmd);
        if (hProcess != NULL)
        {
            WaitForSingleObject(hProcess, INFINITE);
            CloseHandle(hProcess);
        }
    }
    free(lpCmd);
}

// web服务进程监控守护进程是否运行 
static DWORD WINAPI MonitorParentProcess(LPVOID lparam)
{
    DWORD pid = (DWORD)lparam;
    DaemonProcess(pid, TEXT("\" /pid %u"));
    return 0;
}

static DWORD StringToDword(PTCH str)
{
    if ( memicmp(str, TEXT("0x"), 2*sizeof(TCHAR)) == 0 )
    {
        return _tcstol(str+2, NULL, 16);
    }
    else
    {
        return _tcstol(str, NULL, 10);
    }
}


INT _tmain(INT argc, TCHAR* argv[])
{
    WebService();
    return 0;

    /*
    BOOL bRunServer = FALSE;
    DWORD ppid = 0;
    for (int i=1; i<argc; i++)
    {
        if ( 
            memicmp(argv[i], TEXT("/runserver"), 10*sizeof(TCHAR) ) == 0 ||
            memicmp(argv[i], TEXT("-runserver"), 10*sizeof(TCHAR) ) == 0 
            )
        {
            bRunServer = TRUE;
        }
        else if ( 
            memicmp(argv[i], TEXT("/pid"), 4*sizeof(TCHAR) ) == 0 ||
            memicmp(argv[i], TEXT("-pid"), 4*sizeof(TCHAR) ) == 0 
            )
        {
            ppid = StringToDword(argv[++i]);
        }
    }
    if (bRunServer)
    {
        // 启动监控守护进程 
        CloseHandle(CreateThread(NULL, 0, MonitorParentProcess, (LPVOID)ppid, 0, NULL));
        // 启动web服务 
        WebService();
    }
    else
    {
        // 监控web服务 
        DaemonProcess(ppid, TEXT("\" /runserver /pid %u"));
    }
    */
    return 0;
}