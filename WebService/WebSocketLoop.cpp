#include "WebSocketLoop.h"
#include <stdio.h>

// ���� websocket ͨѶ 
void WebSocketHandle_Loop(SOCKET s)
{
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
}
