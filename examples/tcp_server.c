#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main() {
	//Set the server address and port number (6001)
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(struct sockaddr_in));	
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons((unsigned short) 6001);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//Create a "listening" socket, binds it with the above address/port
	int listenSock = socket(AF_INET, SOCK_STREAM, 0);
        int optval = 1;
        setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	bind(listenSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr));	
	listen(listenSock, 16);

	char buf[64];
	while(1) {
			//Accept an incoming connection
			struct sockaddr_in clientAddr;
			socklen_t clientAddrLen = sizeof(clientAddr);			
			int sock = accept(listenSock, (struct sockaddr *)&clientAddr, &clientAddrLen);

			//Read data and print it out
			int n = recv(sock, buf, 64, 0);

			buf[n] = 0;
			printf("Received data: %s\n", buf);
						
			//Close socket
			close(sock);
	}
	return 0;
}

