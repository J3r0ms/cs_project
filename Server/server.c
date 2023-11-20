#include <netinet/in.h> //structure for storing address information 
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/socket.h> //for socket APIs 
#include <sys/types.h> 
#include <ctype.h>
#include <string.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close

#include "cJSON.h"

typedef enum {false, true} bool;

int login(int servSockD){
	return 0;
}

int receive_user_id(int* clientSocket)
{
	char user_id_buffer[300];
	read(*clientSocket, user_id_buffer, 300-1);
	printf("Received user id: %s \n", user_id_buffer);
	int userId = atoi(user_id_buffer);

	return userId;
}

/*
void receive_user_Pass(int* clientSocket)
{
	char password_buffer[300];
	read(*clientSocket, password_buffer, 300-1);
	printf("Received user password: %s \n", password_buffer);

	return password_buffer;

}*/

int verify_user_id(int user_id, int* clientSocket)
{
	FILE* fp = fopen("logs.json", "r");

	// Error Handling
	if (fp == NULL) {
        printf("Error: Unable to open the json log file.\n");
        return 1;
    }

	// reading the full json string from the file fp
    char file_buf[1024] = {0};
    fread(file_buf, 1, sizeof(file_buf), fp);
	fclose(fp);

	// parsing the json string into a cJSON struct
	cJSON *rootArray = cJSON_Parse(file_buf);

	// Error Handling
	if (rootArray == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error: %s\n", error_ptr);
        }
        cJSON_Delete(rootArray);
        return 1;
    }

	bool found_id = false;
	// looping through all logs to find an existing id.
	// If the sent id is found, sends a OK to client
	for(int i = 0; i < cJSON_GetArraySize(rootArray); i++){
		// Get current json object in the logs array
		cJSON *currentLog = cJSON_GetArrayItem(rootArray, i);

		cJSON *currentId = cJSON_GetObjectItem(currentLog, "id");
		cJSON *currentPassword = cJSON_GetObjectItem(currentLog, "password");

		if (user_id == atoi(cJSON_GetStringValue(currentId))){
            found_id = true;
            int msg = 0;
            send(*clientSocket, &msg, sizeof(msg), 0);
            printf("Message was sent: id ok \n");
            break;
			return 0;
        }


		printf("%s \n", cJSON_GetStringValue(currentId));
		printf("%s \n", cJSON_GetStringValue(currentPassword));
		printf("\n");
	}

	if(found_id == false){
		int msg = 1;
		send(*clientSocket, &msg, sizeof(msg), 0);
		printf("Message was sent: id error \n");
		return 1;
	}
}


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
	printf("%d", clientSocket);
	// sends messages to client socket 


	send(clientSocket, serMsg, sizeof(serMsg), 0);


	int user_id = receive_user_id(&clientSocket);
	int error_id = verify_user_id(user_id, &clientSocket);

	if (error_id == 1) {
		int user_id = receive_user_id(&clientSocket);
		int error_id = verify_user_id(user_id, &clientSocket);
	}

	// int error_pass = verify_user_id(receive_user_pass(&clientSocket));
	// if (error_pass == 1)
	// 	int err*or_pass = verify_user_id(receive_user_pass(&clientSocket));



	// receive_user_Pass
	//login(clientSocket);

	return 0; 
}
