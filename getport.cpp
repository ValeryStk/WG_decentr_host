#include "getport.hpp"

// link with Ws2_32.lib
#pragma comment (lib,"Ws2_32.lib")
#pragma comment (lib,"Mswsock.lib")
#pragma comment (lib,"AdvApi32.lib")
#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <stdio.h>

#define SERVER_ADDR "172.217.160.99"
#define SERVER_PORT 80
#pragma warning(disable : 4996)



unsigned int GetFreeUDPPort()
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,* ptr = NULL,hints;
    const char* sendbuf = "this is a test";
    int iResult;



    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }
      
    char myIP[16];
    unsigned int myPort;
    struct sockaddr_in server_addr, my_addr;
    int sockfd;
    //// Connect to server
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Can't open stream socket.");
        exit(-1);
    }

    // Set server_addr
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    server_addr.sin_port = htons(SERVER_PORT);


    //// Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        exit(-1);
    }
  

    // Get my ip address and port
    memset(&my_addr,0, sizeof(my_addr));
    socklen_t len = sizeof(my_addr);
    getsockname(sockfd, (struct sockaddr*)&my_addr, &len);   
    inet_ntop(AF_INET, &my_addr.sin_addr, myIP, sizeof(myIP));   
    myPort = ntohs(my_addr.sin_port);   
    closesocket(sockfd);  
    return myPort;
}