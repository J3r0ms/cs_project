/*
THIS VERSION OF THE CODE DOES NOT ALLOW FOR SIMULTANEOUS CONNECTIONS
*/

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
	cJSON_AddStringToObject(object, "password", pass);

	cJSON *actions_object = cJSON_CreateObject();
	cJSON_AddNumberToObject(actions_object, "delay", delay);

	cJSON *steps_array = cJSON_CreateArray();

	// TODO don't forget to add NULL element at the end of the array
	while (*actions) {
		printf("%s\n", *actions);
		cJSON_AddItemToArray(steps_array, cJSON_CreateString(*actions));
		actions += 1;
	}

	cJSON_AddItemToObject(actions_object, "steps", steps_array);

	cJSON_AddItemToObject(object, "actions", actions_object);

   return object;
}

int write_to_json(cJSON * object)
{
	cJSON *rootArray = parseJson();

	cJSON_AddItemToArray(rootArray, object);

   	char *rootArray_str = cJSON_Print(rootArray); 

	FILE *fp = fopen("logs.json", "w");

	if (fp == NULL) { 
		perror("Error: Unable to open the file.\n"); 
		return 1; 
	}

	printf("%s\n", rootArray_str);
	fputs(rootArray_str, fp);
	fclose(fp);

	return 0;
}

/**
 * This function is called whenever the current user disconnects.
 * Make sure to check for disconnection at every read()
*/
int handle_disconnection(int clientSocket){

	//write_to_json(/* json data */);
	close(clientSocket);
	return 0;
}

int receive_user_id(int clientSocket)
{
	printf("entering receive\n");
	char user_id_buffer[300];
	int bytes_read = read(clientSocket, user_id_buffer, 300-1);
	printf("receiving id: %s\n", user_id_buffer);
	printf("bytes read: %d\n", bytes_read);

	if (bytes_read <= 0) {
		handle_disconnection(clientSocket);
		return -1;
	} 

	printf("Received user id: %s \n", user_id_buffer);
	
	int userId = atoi(user_id_buffer);

	return userId;
}

int verify_user_id(int user_id, int clientSocket)
{
	cJSON *rootArray = parseJson();

	bool found_id = false;
	// looping through all logs to find an existing id.
	// If the sent id is found, sends a OK to client
	int error_code;
	for(int i = 0; i < cJSON_GetArraySize(rootArray); i++){
		// Get current json object in the logs array
		cJSON *currentLog = cJSON_GetArrayItem(rootArray, i);

		cJSON *currentId = cJSON_GetObjectItem(currentLog, "id");

		if (user_id == atoi(cJSON_GetStringValue(currentId))){
            found_id = true;
            int msg = 0;
            error_code = send(clientSocket, &msg, sizeof(msg), 0);
			if (error_code == -1) {
				handle_disconnection(clientSocket);
				return -1;
			} 

            printf("Message was sent: id ok \n");
			return 0;
        }
	}

	if(found_id == false){
		int msg = 1;
		error_code = send(clientSocket, &msg, sizeof(msg), 0);
		if (error_code == -1) {
			handle_disconnection(clientSocket);
			return -1;
		} 
		printf("Message was sent: id not found, asking for a new password \n");
		return 1;
	}

	return -1;
}

int receive_password(int clientSocket, char **pass)
{

	char password_buffer[300];
	int bytes_read = read(clientSocket, password_buffer, 300-1);
	if (bytes_read <= 0) {
		handle_disconnection(clientSocket);
		return -1;
	} 
	
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

int receive_amount(int clientSocket)
{
	char amount_buffer[300];
	int bytes_read = read(clientSocket, amount_buffer, 300-1);
	if (bytes_read <= 0) {
		handle_disconnection(clientSocket);
		return -1;
	} 

	return atoi(amount_buffer);
}

int verify_password(int user_id, char* user_password, int clientSocket)
{

	cJSON *rootArray = parseJson();

	int error_code;
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
				error_code = send(clientSocket, &msg, sizeof(msg), 0);
				if (error_code == -1) {
					handle_disconnection(clientSocket);
					return -1;
				} 

				return 0; // passwords are equal
			}
			
            int msg = 1;
            send(clientSocket, &msg, sizeof(msg), 0);
			if (error_code == -1) {
				handle_disconnection(clientSocket);
				return -1;
			} 

            printf("Wrong password \n");
			return 1;
        }

	}
	
	return -1;
}


int login_user(int clientSocket, int *id, char** pass){
	int user_id = receive_user_id(clientSocket);
	if (user_id == -1) {
		printf("if layer 2\n");
		return -1;
	}

	int id_error_code = verify_user_id(user_id, clientSocket); // 0 -> user exists, 1 -> user not known
	if (id_error_code == -1) return -1;	

	char *password = NULL;

	int error_code;
	// user exists
	if (id_error_code == 0) {
		
		printf("waiting for password \n");
		error_code = receive_password(clientSocket, &password);
		if (error_code == -1) return -1;

		int password_error_code = verify_password(user_id, password, clientSocket);
		if (password_error_code == -1) return -1;

		while(password_error_code == 1){
			printf("waiting for password \n");
			error_code = receive_password(clientSocket, &password);
			if (error_code == -1) return -1;

			password_error_code = verify_password(user_id, password, clientSocket);
			if (password_error_code == -1) return -1;

		}

	}
	// user not yet know, create a new password
	else if (id_error_code == 1){
		error_code = receive_password(clientSocket, &password);
		if (error_code == -1) return -1;
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

char** createActionsArray(int* amountsArray, int index) {
    char** actionsArray = (char**)malloc(index * sizeof(char*));

    if (actionsArray == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < index; i++) {
        // Allocate memory for the action string
        actionsArray[i] = (char*)malloc(30);  // Adjust the size based on your expected string length

        if (actionsArray[i] == NULL) {
            fprintf(stderr, "Memory allocation failed.\n");
            exit(EXIT_FAILURE);
        }

        // Construct the action string based on the corresponding amount
        if (amountsArray[i] > 0) {
            snprintf(actionsArray[i], 30, "INCREASE BY %d", amountsArray[i]);
        } else if (amountsArray[i] < 0) {
            snprintf(actionsArray[i], 30, "DECREASE BY %d", -amountsArray[i]);
        } else {
            actionsArray[i] = NULL;
        }
    }

    return actionsArray;
}

int* receive_all_amounts(int clientSocket, int *amountArraySize) {
    int* amountsArray = (int*)malloc(sizeof(int));  // Allocate memory for the first element

    if (amountsArray == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    int index = 0;
    int amount;

    do {
        // Receive an amount from the client
        amount = receive_amount(clientSocket);

        // Reallocate the array to accommodate the new amount
        amountsArray = (int*)realloc(amountsArray, (index + 1) * sizeof(int));

        if (amountsArray == NULL) {
            fprintf(stderr, "Memory reallocation failed.\n");
            exit(EXIT_FAILURE);
        }

        // Store the received amount in the array
        amountsArray[index] = amount;

        // Print the received amount
        printf("Received amount: %i \n", amount);

        // Increment the index for the next iteration
        index++;
    } while (amount != 0);

	*amountArraySize = index;

    return amountsArray;
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
	
	printf("Listening for connections... \n");

	while(1){
		printf("\nWaiting for the next connection... \n\n");
			// integer to hold client socket. 
		int clientSocket = accept(servSockD, NULL, NULL);
		printf("Client %d is now connected\n", clientSocket);
		// sends messages to client socket 

		// Initial (dummy) message
		int error_code = send(clientSocket, serMsg, sizeof(serMsg), 0);
		if (error_code == -1) continue; // accept the next connection

		int user_id;
		char *password;

		// Step 1: LOGIN
		int login_error_code = login_user(clientSocket, &user_id, &password);
		if (login_error_code == -1) {
			printf("if layer 3\n");
			// TODO Move free() to handle_disconnection once the struct is created.	
			//free(password);
			handle_disconnection(clientSocket);
			continue;
		}

		printf("user: %d \n", user_id);
		printf("pass: %s \n", password);
		
		printf("login successful \n");

		int amounts_array_length;
		int* amounts_array = receive_all_amounts(clientSocket, &amounts_array_length);

		char** actions_array = createActionsArray(amounts_array, amounts_array_length);

		// int delay = get_delay();
		// send(clientSocket, &delay, sizeof(delay), 0);

		write_to_json(generate_log_object(user_id, password, 3, actions_array));

		free(password);
		free(amounts_array);
		free(actions_array);
	}

	return 0; 
}
