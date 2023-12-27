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
#include <math.h>

#include "cJSON.h"

typedef enum {false, true} bool;

typedef struct {
	int clientSocket;
	int id;
	char *password;
	int *amountsArray;
	int amountsArrayLength;
} UserData;

void check_and_create_log_file() {
    FILE* file;

    // Try to open the file in read mode
    file = fopen("logs.json", "r");

    // Check if the file exists
    if (file) {
        // File exists, check if it is empty
        fseek(file, 0, SEEK_END);  // Move to the end of the file
        long fileSize = ftell(file);  // Get the size of the file

        if (fileSize == 0) {
            // File is empty, write an empty JSON array
            fclose(file);  // Close the file first
            file = fopen("logs.json", "w");
            if (file == NULL) {
                perror("Error: Unable to open the log file to write empty array");
                exit(EXIT_FAILURE);
            }
            fputs("[]", file);
        }

        fclose(file);
    } else {
        // File doesn't exist, create it with an empty JSON array
        file = fopen("logs.json", "w");
        if (file == NULL) {
            perror("Error: Unable to create the log file");
            exit(EXIT_FAILURE);
        }

        fputs("[]", file);
        fclose(file);
    }
}

cJSON* parseJson(){
	FILE* fp = fopen("logs.json", "r");

	// Error Handling
	if (fp == NULL) {
        printf("Error: Unable to open the json log file.\n");
        return NULL;
    }

	// reading the full json string from the file fp
    char fileBuf[1024] = {0};
    fread(fileBuf, 1, sizeof(fileBuf), fp);
	fclose(fp);

	// parsing the json string into a cJSON struct
	cJSON *rootArray = cJSON_Parse(fileBuf);

	// Error Handling
	if (rootArray == NULL) {
        const char *errorPtr = cJSON_GetErrorPtr();
        if (errorPtr != NULL) {
            printf("Parse Error: %s\n", errorPtr);
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

	cJSON *actionsObject = cJSON_CreateObject();
	cJSON_AddNumberToObject(actionsObject, "delay", delay);

	cJSON *stepsArray = cJSON_CreateArray();

	// TODO don't forget to add NULL element at the end of the array
	while (*actions) {
		printf("%s\n", *actions);
		cJSON_AddItemToArray(stepsArray, cJSON_CreateString(*actions));
		actions += 1;
	}

	cJSON_AddItemToObject(actionsObject, "steps", stepsArray);

	cJSON_AddItemToObject(object, "actions", actionsObject);

   return object;
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
int handle_disconnection(UserData *userData) {

	if (userData->amountsArray != NULL){

		char** actions_array = createActionsArray(userData->amountsArray, userData->amountsArrayLength);

		write_to_json(generate_log_object(userData->id, userData->password, 3, actions_array));

		free(userData->amountsArray);
		free(actions_array);
	}
	if (userData->password != NULL) free(userData->password);

	close(userData->clientSocket);
	return 0;
}

int receive_user_id(UserData *userData)
{
	printf("entering receive\n");
	char userIdBuffer[300];
	int bytesRead = read(userData->clientSocket, userIdBuffer, 300-1);
	printf("receiving id: %s\n", userIdBuffer);
	printf("bytes read: %d\n", bytesRead);

	if (bytesRead <= 0) {
		handle_disconnection(userData);
		return -1;
	} 

	printf("Received user id: %s \n", userIdBuffer);
	
    userIdBuffer[bytesRead] = '\0'; // Ensure null-termination for safe conversion
	int userId = atoi(userIdBuffer);

	userData->id = userId;

	return 0;
}

int verify_user_id(UserData *userData)
{
	cJSON *rootArray = parseJson();
	if(rootArray == NULL) printf("root array is null\n");
	printf("array size: %d\n", cJSON_GetArraySize(rootArray));

	bool foundId = false;
	// looping through all logs to find an existing id.
	// If the sent id is found, sends a OK to client
	for(int i = 0; i < cJSON_GetArraySize(rootArray); i++){
		// Get current json object in the logs array
		cJSON *currentLog = cJSON_GetArrayItem(rootArray, i);

		cJSON *currentId = cJSON_GetObjectItem(currentLog, "id");
		
		if(currentId == NULL) {
			printf("current id is null\n");
			continue; // Skip this iteration if currentId is NULL
		}

		const double idInt = cJSON_GetNumberValue(currentId);
		
		if(isnan(idInt)) {
			printf("current id nan\n");
			continue; // Skip this iteration if idInt is nan
		}

		if (userData->id == (int) idInt){
            foundId = true;
            int msg = 0;

			if (send(userData->clientSocket, &msg, sizeof(msg), 0) == -1) {
				handle_disconnection(userData);
				return -1;
			} 

            printf("Message was sent: id ok \n");
			return 0;
        }
	}

	if(foundId == false){
		int msg = 1;
		if (send(userData->clientSocket, &msg, sizeof(msg), 0) == -1) {
			handle_disconnection(userData);
			return -1;
		} 
		printf("Message was sent: id not found, asking for a new password \n");
		return 1;
	}

	return -1;
}

int receive_password(UserData *userData)
{

	char passwordBuffer[300];

	if (read(userData->clientSocket, passwordBuffer, 300-1) <= 0) {
		handle_disconnection(userData);
		return -1;
	} 
	
	printf("Received password: %s \n", passwordBuffer);

    size_t len = strlen(passwordBuffer);

	userData->password = (char*)malloc(len + 1);
	if (userData->password == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

	strcpy(userData->password, passwordBuffer);

	return 0;
}

int receive_amount(UserData *userData, int* amount) 
{
    char amountBuffer[300];
    int bytes_read = read(userData->clientSocket, amountBuffer, 299);  // Reads up to 299 bytes

    if (bytes_read <= 0) {
        handle_disconnection(userData);
        return -1;  // Return -1 in case of disconnection or read error
    } 

    amountBuffer[bytes_read] = '\0'; // Ensure null-termination for safe conversion
    *amount = atoi(amountBuffer);    // Convert the buffer to an integer

    return 0;  // Return 0 on successful completion
}


int verify_password(UserData *userData)
{
	cJSON *rootArray = parseJson();

	for(int i = 0; i < cJSON_GetArraySize(rootArray); i++){
		// Get current json object in the logs array
		cJSON *currentLog = cJSON_GetArrayItem(rootArray, i);

		if(currentLog == NULL) {
			printf("current log is null\n");
			continue; // Skip this iteration if currentLog is NULL
		}

		cJSON *currentId = cJSON_GetObjectItem(currentLog, "id");
		cJSON *currentPassword = cJSON_GetObjectItem(currentLog, "password");

		if(currentId == NULL || currentPassword == NULL) {
			printf("current id or password is null\n");
			continue; // Skip this iteration if currentId or currentPassword is NULL
		}

		const double idInt = cJSON_GetNumberValue(currentId);
		
		if(isnan(idInt)) {
			printf("current id nan\n");
			continue; // Skip this iteration if idInt is nan
		}

		if (userData->id == (int) idInt){

			printf("user pass: %s \n", userData->password);
			printf("json pass: %s \n", cJSON_GetStringValue(currentPassword));

			if(strcmp(userData->password, cJSON_GetStringValue(currentPassword)) == 0){
				printf("passwords are the same! \n");
				int msg = 0;
				if (send(userData->clientSocket, &msg, sizeof(msg), 0) == -1) {
					handle_disconnection(userData);
					return -1;
				} 

				return 0; // passwords are equal
			}
			
            int msg = 1;
			if (send(userData->clientSocket, &msg, sizeof(msg), 0) == -1) {
				handle_disconnection(userData);
				return -1;
			} 

            printf("Wrong password \n");
			return 1;
        }

	}
	
	return -1;
}


int login_user(UserData *userData){
	int error = receive_user_id(userData);
	if (error == -1) {
		printf("if layer 2\n");
		return -1;
	}

	int id_error_code = verify_user_id(userData); // 0 -> user exists, 1 -> user not known
	if (id_error_code == -1) return -1;

	char *password = NULL;
	// user exists
	if (id_error_code == 0) {
		
		printf("waiting for password \n");
		if (receive_password(userData) == -1) return -1;

		int password_error_code = verify_password(userData);
		if (password_error_code == -1) return -1;

		while(password_error_code == 1){
			printf("waiting for password \n");
			if (receive_password(userData) == -1) return -1;

			password_error_code = verify_password(userData);
			if (password_error_code == -1) return -1;

		}

	}
	// user not yet know, create a new password
	else if (id_error_code == 1){
		if (receive_password(userData) == -1) return -1;
	}
	else{
		printf("Error in verifying user id: %d", id_error_code);
	}

	return 0;
}



int receive_all_amounts(UserData *userData) {
    // Initialize the array pointer to NULL
	int clientSocket = userData->clientSocket;

	userData->amountsArray = NULL;

    int index = 0;
	int receivedAmount;

    do {
        // Receive an amount from the client
        int error = receive_amount(userData, &receivedAmount);
        if (error == -1) return -1;

        // Reallocate the array to accommodate the new amount
        int* tempArray = (int*)realloc(userData->amountsArray, (index + 1) * sizeof(int));
        if (tempArray == NULL) {
			handle_disconnection(userData);
            fprintf(stderr, "Memory reallocation failed.\n");
            return -1;  
        }

		userData->amountsArray = tempArray;
		userData->amountsArray[index] = receivedAmount;

        // Print the received amount
        printf("Received amount: %i \n", receivedAmount);

        // Increment the index for the next iteration
        index++;
    } while (receivedAmount != 0);

	userData->amountsArrayLength = index;

    return 0;  // Return 0 on successful completion
}


int main(int argc, char const* argv[]) 
{ 	
	// Create the logs.json file if it doesn't exist
	check_and_create_log_file();

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

		UserData userData;
		
		userData.amountsArray = NULL;
		userData.password = NULL;

		userData.clientSocket = accept(servSockD, NULL, NULL);
		printf("Client %d is now connected\n", userData.clientSocket);
		// sends messages to client socket 

		// Initial (dummy) message
		int error_code = send(userData.clientSocket, serMsg, sizeof(serMsg), 0);
		if (error_code == -1) continue; // accept the next connection

		// Step 1: LOGIN
		int login_error_code = login_user(&userData);
		if (login_error_code == -1) {
			printf("if layer 3\n");
			// TODO Move free() to handle_disconnection once the struct is created.	
			//free(password);
			continue;
		}

		printf("user: %d \n", userData.id);
		printf("pass: %s \n", userData.password);
		
		printf("login successful \n");

		int error = receive_all_amounts(&userData);
		if (error == -1) continue; // accept the next connection

		// After the user has finished sending all the amounts, disconnect
		// e.i. save the amounts to json, free the variables and close the socket
		handle_disconnection(&userData);
	}

	return 0; 
}
