// 入口及socket连接过程 

#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include "AnalyzeRequestHttp.h"
#include "ServiceProvider.h"

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

int main(int argc, char* argv[])
{
    WebService();
    return 0;
}