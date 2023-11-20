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

cJSON* parseJson(){
	FILE* fp = fopen("logs.json", "r");

	// Error Handling
	if (fp == NULL) {
        printf("Error: Unable to open the json log file.\n");
        return NULL;
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
        return NULL;
    }

	return rootArray;
}

cJSON* generate_log_object(int id, char *pass, int delay, char **actions)
{
	cJSON *object = cJSON_CreateObject();
	cJSON_AddNumberToObject(object, "id", id);
	cJSON_AddStringToObject(object, "password", *pass);

	cJSON *actions_object = cJSON_CreateObject();
	cJSON_AddStringToObject(object, "delay", delay);

	cJSON *steps_array = cJSON_CreateArray();

	// TODO don't forget to add NULL element at the end of the array
	while (*actions) {
		printf("%s\n", *actions);
		cJSON_AddItemToArray(steps_array, *actions);
		actions += 1;
	}

	cJSON_AddArrayToObject(object, steps_array);

   return object;
}

void write_to_json(cJSON * object)
{
	cJSON *rootArray = parseJson();

	rootArray = cJSON_AddItemToArray(rootArray, object);

	FILE *fp = fopen("logs.json", "w");

	if (fp == NULL) { 
		printf("Error: Unable to open the file.\n"); 
		return 1; 
	}

	printf("%s\n", rootArray);
	fputs(rootArray, fp);
	fclose(fp);
}

int receive_user_id(int clientSocket)
{
	char user_id_buffer[300];
	read(clientSocket, user_id_buffer, 300-1);
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

int verify_user_id(int user_id, int clientSocket)
{
	cJSON *rootArray = parseJson();

	bool found_id = false;
	// looping through all logs to find an existing id.
	// If the sent id is found, sends a OK to client
	for(int i = 0; i < cJSON_GetArraySize(rootArray); i++){
		// Get current json object in the logs array
		cJSON *currentLog = cJSON_GetArrayItem(rootArray, i);

		cJSON *currentId = cJSON_GetObjectItem(currentLog, "id");

		if (user_id == atoi(cJSON_GetStringValue(currentId))){
            found_id = true;
            int msg = 0;
            send(clientSocket, &msg, sizeof(msg), 0);
            printf("Message was sent: id ok \n");
			return 0;
        }
	}

	if(found_id == false){
		// If the id was not found: create a new one

		// -- TODO --
		//create_user(clientSocket);
		// ----------

		int msg = 1;
		send(clientSocket, &msg, sizeof(msg), 0);
		printf("Message was sent: id not found, asking for a new password \n");
		return 1;
	}

	return -1;
}

int receive_password(int clientSocket, char **pass){

	char password_buffer[300];
	read(clientSocket, password_buffer, 300-1);
	printf("Received password: %s \n", password_buffer);

    size_t len = strlen(password_buffer);

	*pass = (char*)malloc(len + 1);
	if (*pass == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

	strcpy(*pass, password_buffer);

	return 0;
}

int verify_password(int user_id, char* user_password, int clientSocket){

	cJSON *rootArray = parseJson();

	for(int i = 0; i < cJSON_GetArraySize(rootArray); i++){
		// Get current json object in the logs array
		cJSON *currentLog = cJSON_GetArrayItem(rootArray, i);

		cJSON *currentId = cJSON_GetObjectItem(currentLog, "id");
		cJSON *currentPassword = cJSON_GetObjectItem(currentLog, "password");

		if (user_id == atoi(cJSON_GetStringValue(currentId))){

			printf("user pass: %s \n", user_password);
			printf("json pass: %s \n", cJSON_GetStringValue(currentPassword));

			if(strcmp(user_password, cJSON_GetStringValue(currentPassword)) == 0){
				printf("passwords are the same! \n");
				int msg = 0;
				send(clientSocket, &msg, sizeof(msg), 0);
				return 0; // passwords are equal
			}
			
            int msg = 1;
            send(clientSocket, &msg, sizeof(msg), 0);
            printf("Wrong password \n");
			return 1;
        }

	}
	
	return -1;
}


int login_user(int clientSocket, int *id, char** pass){
	int user_id = receive_user_id(clientSocket);
	int id_error_code = verify_user_id(user_id, clientSocket); // 0 -> user exists, 1 -> user not known

	char *password = NULL;

	// user exists
	if (id_error_code == 0) {
		
		printf("waiting for password \n");
		receive_password(clientSocket, &password);
		int password_error_code = verify_password(user_id, password, clientSocket);

		if(password_error_code == -1) printf("Error in verifying password: %d", password_error_code);

		while(password_error_code == 1){
		printf("waiting for password \n");
		receive_password(clientSocket, &password);
			password_error_code = verify_password(user_id, password, clientSocket);

		}

	}
	// user not yet know, create a new password
	else if (id_error_code == 1){
		receive_password(clientSocket, &password);
	}
	else{
		printf("Error in verifying user id: %d", id_error_code);
	}

	
    size_t len = strlen(password);

	*pass = (char*)malloc(len + 1);
	if (*pass == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

	strcpy(*pass, password);

	*id = user_id;

	return 0;
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

	// Initial (dummy) message
	send(clientSocket, serMsg, sizeof(serMsg), 0);

	int user_id;
	char *password;

	// Step 1: LOGIN
	login_user(clientSocket, &user_id, &password);

	printf("user: %d \n", user_id);
	printf("pass: %s \n", password);
	
	printf("login successful \n");

	// int error_pass = verify_user_id(receive_user_pass(&clientSocket));
	// if (error_pass == 1)
	// 	int err*or_pass = verify_user_id(receive_user_pass(&clientSocket));



	// receive_user_Pass
	//login(clientSocket);

	return 0; 
}
