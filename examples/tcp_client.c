#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
	//The message to be sent to the server
	const char * msg = "Hello World!";

	//Set the destination IP address and port number
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons((unsigned short) 6001);
	inet_pton(AF_INET, "52.90.167.79", &serverAddr.sin_addr);
	
	//Create the socket, connect to the server
	int sock = socket(AF_INET, SOCK_STREAM, 0);	
	connect(sock, (const struct sockaddr *) &serverAddr, sizeof(serverAddr));
		
	//Send data
	send(sock, msg, strlen(msg), 0);	

	//Close socket
	close(sock);
	return 0;
}

