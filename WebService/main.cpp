// ��ڼ�socket���ӹ��� 

#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32.lib")

// httpЭ����� 
DWORD CALLBACK DoService(LPVOID lpThreadParameter)
{
    __try
    {
        DWORD dwsize = 4096;
        SOCKET se = (SOCKET)lpThreadParameter;
        char* buf=(char*)malloc(dwsize);

        // �������� 
        ZeroMemory(buf, dwsize);
        recv(se, buf, dwsize, 0);

        // ����ͷ��Ϣ 

        // ���ö�Ӧ����Դ�ļ� 

        // ����ͷ�� 

        // ����body 

        // �ر����� 
        closesocket(se);
        free(buf);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        printf("something is error\n");
    }
    return 0;
}

// ����80�˿ڽ��з��� 
INT WebService()
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

int main(int argc, char* argv[])
{
    WebService();
    return 0;
}