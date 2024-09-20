// ��ڼ�socket���ӹ��� 

#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include "AnalyzeRequestHttp.h"
#include "ServiceProvider.h"
#include "common.h"
#include <Windows.h>
#include <tchar.h>

#pragma comment(lib, "ws2_32.lib")

// httpЭ����� 
static DWORD CALLBACK DoService(LPVOID lpThreadParameter)
{
    DWORD dwsize = 4096;
    SOCKET s = (SOCKET)lpThreadParameter;
    char* buf=(char*)malloc(dwsize);

    // ��������,����ͷ��Ϣ 
    ZeroMemory(buf, dwsize);
    RequestHeaderInfo request(s);
//     printf("%s\n", request.GetUserAgent());

    // ���ö�Ӧ����Դ�ļ�,�ظ���Ϣ 
    ResponseData(s, &request);

    // �ر����� 
    closesocket(s);
    free(buf);
    return 0;
}

// ����80�˿ڽ��з��� 
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
                // �󶨱���80�˿� 
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

                // ����ͬʱ�ɷ���20������ 
                if (SOCKET_ERROR == listen(ss, 20))
                {
                    printf("listen call error: %d\n", WSAGetLastError());
                    break;
                }

                bRetVal = 0;
                struct sockaddr_in so = {0};
                while(TRUE)
                {
                    // �ɹ����ӿͻ����������̴߳������ 
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

// ���н��� 
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

// �ػ����� 
static void DaemonProcess(DWORD pid, LPTSTR lpcmdline)
{
    if (pid != 0)
    {
        // ���н��̵ȴ� 
        HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);
        WaitForSingleObject(hProcess, INFINITE);
        CloseHandle(hProcess);
    }

    // �������̲��ȴ� 
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

// web������̼���ػ������Ƿ����� 
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
        // ��������ػ����� 
        CloseHandle(CreateThread(NULL, 0, MonitorParentProcess, (LPVOID)ppid, 0, NULL));
        // ����web���� 
        WebService();
    }
    else
    {
        // ���web���� 
        DaemonProcess(ppid, TEXT("\" /runserver /pid %u"));
    }
    */
    return 0;
}