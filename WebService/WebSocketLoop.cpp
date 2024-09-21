#include "WebSocketLoop.h"
#include <stdio.h>

// 处理 websocket 通讯 
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
