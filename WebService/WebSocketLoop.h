#ifndef _WEB_SOCKET_MESSAGE_HEADER_HH_H_
#define _WEB_SOCKET_MESSAGE_HEADER_HH_H_
#include <WinSock2.h>
#define MALLOC(_a)  malloc(_a)
#define MFREE(_a)  if(_a) {free(_a); _a=NULL;}

#ifdef _cplusplus
extern "C" {
#endif // _cplusplus 


// ���ͽ���websocket ��Ϣ 
extern char* recv_websocket(SOCKET s);
extern void send_websocket(SOCKET s, char* data, unsigned long len);


// websocket ������ѭ�� 
void WebSocketHandle_Loop(SOCKET s);


#ifdef _cplusplus
};
#endif // _cplusplus 

#endif //  _WEB_SOCKET_MESSAGE_HEADER_HH_H_ 