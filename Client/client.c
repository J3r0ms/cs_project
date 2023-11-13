#include <netinet/in.h> //structure for storing address information 
#include <stdio.h> // for printf()
#include <stdlib.h> // for exit()
#include <sys/socket.h> //for socket APIs 
#include <sys/types.h> //for data types

int main(int argc, char const* argv[]) 
{ 
	int sockD = socket(AF_INET, SOCK_STREAM, 0); // Internal client socket

	struct sockaddr_in servAddr; // Server address

	servAddr.sin_family = AF_INET; // IPv4
	servAddr.sin_port // Port number
		= htons(9001); // use some unused port number 
	servAddr.sin_addr.s_addr = INADDR_ANY; // IP address

	int connectStatus 
		= connect(sockD, (struct sockaddr*)&servAddr, 
				sizeof(servAddr)); // Connect to server

	if (connectStatus == -1) { // Error handling
		printf("Error...\n"); 
	} 

	else { // Actions here

	// In this small DEMO : we will receive a message from the server

		char strData[255]; 

		recv(sockD, strData, sizeof(strData), 0); 

		printf("Message: %s\n", strData); 
	} 

	return 0; 
}

