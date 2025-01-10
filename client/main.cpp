#include<WinSock2.h>
#include<WS2tcpip.h>
#include<Windows.h>
#include<iostream>

#pragma comment(lib, "Ws2_32.lib") 

#define BUF_SIZE 1024

using namespace std;

char szMsg[BUF_SIZE];

unsigned SendMsg(void *arg) {
    SOCKET sock = *((SOCKET *)arg);

    while (1) {
        cin >> szMsg;
        if (!strcmp(szMsg, "EXIT\n") || !strcmp(szMsg, "exit\n")) {
            closesocket(sock);
            exit(0);
        }

        //send(sock, szMsg, strlen(szMsg), 0);
        send(sock, szMsg, static_cast<int>(strlen(szMsg)), 0);

    }

    return 0;
}

unsigned RecvMsg(void* arg) {
    SOCKET sock = *((SOCKET*)arg);
    char msg[BUF_SIZE];

    while (1) {
        int len = recv(sock, msg, sizeof(msg) - 1, 0);
        if (len == -1) {
            return -1;
        }
        msg[len] = '\0';
        
        printf("%s\n", msg);
    }

    return 0;
}



int main() {
	//初始化windows socket环境
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        return -1;
    }
    /* Confirm that the WinSock DLL supports 2.2.*/
    /* Note that if the DLL supports versions greater    */
    /* than 2.2 in addition to 2.2, it will still return */
    /* 2.2 in wVersion since that is the version we      */
    /* requested.                                        */

    if (LOBYTE(wsaData.wVersion) != 2 ||
        HIBYTE(wsaData.wVersion) != 2) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        WSACleanup();
        return -1;
    }

    /* The WinSock DLL is acceptable. Proceed. */

    

    //创建通信socket
    SOCKET hSock;
    hSock = socket(AF_INET, SOCK_STREAM, 0);

    //绑定服务器IP和端口
    SOCKADDR_IN servAdr;
    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_port = htons(9999);
    inet_pton(AF_INET, "114.55.108.251", &servAdr.sin_addr);
    
    //连接服务器
    if (connect(hSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR) {
        printf("connect error: %d\n", GetLastError());
        return -1;
    }
    else {
        printf("欢迎来到瓜皮聊天室，请输入你的昵称：");
    }

    //创建两个线程 分别循环发消息和循环收消息
    HANDLE hSendHand = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendMsg, (void *)&hSock, 0, NULL);
    HANDLE hRecvHand = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RecvMsg, (void *)&hSock, 0, NULL);

    //等待线程结束
    WaitForSingleObject(hSendHand, INFINITE);
    WaitForSingleObject(hRecvHand, INFINITE);

    //释放套接字
    closesocket(hSock);
    WSACleanup();

    return 0;
}