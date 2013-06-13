#include "stdafx.h"
#include "udpThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#pragma  comment (lib, "ws2_32.lib")

#define MSG_BUFFER_LENGTH (2048) //接受buffer长度
#define UDP_LISTENING_PORT (9160)

char recvbuf[MSG_BUFFER_LENGTH]; 

DWORD WINAPI UdpThread(LPVOID lpParam)
{
    WSADATA   wsd;
	SOCKET    recv_s;
	//char      *recvbuf;
	int       ret;
	int       port;
	SOCKADDR_IN localaddr;
	SOCKADDR_IN destaddr;
	int destlen = sizeof(destaddr);

	port = UDP_LISTENING_PORT;
    
	//初始化Winsock库
	if(WSAStartup(MAKEWORD(2,2),&wsd) != 0)
	{
		printf("WSAStartup error!\n");
		exit(1);
	}
    //创建UDP套接字
	recv_s = socket(AF_INET, SOCK_DGRAM, 0);
	if(recv_s == INVALID_SOCKET)
	{
		printf("socket() errror : %d\n",WSAGetLastError());
		exit(1);
	}

    //对本机地址结构进行设置
	localaddr.sin_family = AF_INET;
	localaddr.sin_port = htons((u_short)port);
	localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
    //对套接字进行绑定
	if(bind(recv_s, (SOCKADDR *)&localaddr, sizeof(localaddr)) == SOCKET_ERROR)
	{
		printf("bind() error: %d\n", WSAGetLastError());
		exit(1);
	}

	printf("在端口%d监听...\n",port);

	int recvlen = MSG_BUFFER_LENGTH;
    //开始接受消息
	while(1)
	{
		ret = recvfrom(recv_s, recvbuf, recvlen, 0,(SOCKADDR*)&destaddr, &destlen);//此函数为阻塞函数

		if(ret == SOCKET_ERROR)
		{
			printf("recvfrom() error: %d\n",WSAGetLastError());
			exit(1);
		}
		else
		{
            if( ret != sizeof(GlbCommand) )
            {
                printf("无法解析命令\n");
            }
            else
            {
			    printf("收到主机 %s 的消息:\n",inet_ntoa(destaddr.sin_addr));
                SparseCmd((void*)recvbuf);
            }
		}

	}
	closesocket(recv_s);
	WSACleanup();
	return 0;
}

void SparseCmd(void* buf)
{
    GlbCommand* cmdBuf = (GlbCommand*)buf;

    switch(cmdBuf->command)
    {
    case GLB_CMD_PLAY:
        printf("play\n");
        break;
    case GLB_CMD_PAUSE:
        printf("pause\n");
        break;
    }

}