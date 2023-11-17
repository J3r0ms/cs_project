#include <netinet/in.h> //structure for storing address information 
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/socket.h> //for socket APIs 
#include <sys/types.h> 
#include <stdio.h> 
#include <cjson/cJSON.h>


void receiving_password(int servSockD)
{
		char password[255];
		recv(servSockD,password,sizeof(password), 0);

		if(!(strcmp(json->valuestring, password) == 0)){
			char passMSG[255] = "Message from the server to the "
					"Password not found"; 
			send(servSockD,passMSG,sizeof(passMSG),0);
		}
		else if(strcmp(json->valuestring, password) == 1){
			//TODO
			// il faut que ankit termine d'abord le JSON file

		}
		else if(strcmp(json->valuestring, password) =>2){
			

		}
		cJSON *json = cJSON_CreateObject(); 
   		cJSON_AddStringToObject(json, "password", password);

		
}

int main(int argc, char const* argv[]) 
{ 

	// create server socket similar to what was done in 
	// client program 
	int servSockD = socket(AF_INET, SOCK_STREAM, 0); 

	// string store data to send to client 
	char serMsg[255] = "Message from the server to the "
					"client \'Hello Client\' "; 

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
	receiving_password(servSockD);

	
	return 0; 
}

