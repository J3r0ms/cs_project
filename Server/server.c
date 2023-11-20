#include <netinet/in.h> //structure for storing address information 
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/socket.h> //for socket APIs 
#include <sys/types.h> 
#include <ctype.h>
#include <string.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close


int main(int argc, char const* argv[]) 
{ 
	// create server socket similar to what was done in 
	// client program 
	int servSockD = socket(AF_INET, SOCK_STREAM, 0); 
	// string store data to send to client 
	char serMsg[255] = "Successfully connected to server.";
	// define server address 
	struct sockaddr_in servAddr; 
	servAddr.sin_family = AF_INET; 
	servAddr.sin_port = htons(9001); 
	servAddr.sin_addr.s_addr = INADDR_ANY; 
	// bind socket to the specified IP and port 
	bind(servSockD, (struct sockaddr*)&servAddr, 
		sizeof(servAddr)); 

	// listen for connections 
	listen(servSockD, 1); 
	// integer to hold client socket. 
	int clientSocket = accept(servSockD, NULL, NULL); 
	// sends messages to client socket 
	send(clientSocket, serMsg, sizeof(serMsg), 0);

	char buffer[300];
	read(clientSocket, buffer, 300-1);
	printf("Received user id: %s \n", buffer);

	char buffer2[300];
	read(clientSocket, buffer2, 300-1);
	printf("Received user password: %s \n", buffer2);



	return 0; 
}
