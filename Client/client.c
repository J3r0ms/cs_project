#include <netinet/in.h> //structure for storing address information
#include <stdio.h> // for printf()
#include <stdlib.h> // for exit()
#include <sys/socket.h> //for socket APIs
#include <sys/types.h> //for data types
#include <ctype.h>
#include <string.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close

#define MAX_INT 2147483647
#define MAX_LENGTH 200

int send_ID(int sockD)
{
	char userID[MAX_LENGTH];

	printf("Enter user id: \n");
	if (fgets(userID, sizeof(userID), stdin) == NULL) {
		printf("An error occured, please try again. \n");
		while ((getchar()) != '\n');
		return -1;
	}

	size_t length = strlen(userID);
	if (length <= 0 || userID[length-1] != '\n') {
		printf("Error, nothing was received or the input is too big. \n");
		while ((getchar()) != '\n');
		return -1;
	}

	char* end_pointer;
	long id = strtol(userID, &end_pointer, 10);

    if ((end_pointer == userID) || (*end_pointer != '\0' && *end_pointer != '\n') ||
		id < 0 || id > MAX_INT) {
        printf("Error, please provide a valid integer. \n");
		return -1;
    }

	send(sockD, (int) id, sizeof((int) id), 0);
	return 0;
}

int send_Password(int sockD)
{
		char userPass[MAX_LENGTH];

		printf("Enter user password: \n");
		if (fgets(userPass, sizeof(userPass), stdin) == NULL) {
			printf("An error occured, your password is too long. \n");
			while ((getchar()) != '\n');
			return -1;
		}

		size_t pass_length = strlen(userPass);
		if (pass_length <= 0 || userPass[pass_length-1] != '\n') {
			printf("Error, nothing was received or the input is too big. \n");
			while ((getchar()) != '\n');
			return -1;
		}

		//TODO shouldn't be possible to not have password

		send(sockD, userPass, sizeof(userPass), 0);
		return 0;
}

int give_counter_choice(int sockD)
{
	char input[100];
	if (fgets(input, sizeof(input), stdin) == NULL) {
		printf("An error occured, please try again");
		while ((getchar()) != '\n');
		return -1;
	}

	size_t length = strlen(input);
	if (length <= 0 || input[length-1] != '\n') {
		printf("Error, nothing was received or the input is too big. \n");
		while ((getchar()) != '\n');
		return -1;
	}

	char* end_pointer;
	long id = strtol(input, &end_pointer, 10);
    if ((end_pointer == input) || (*end_pointer != '\0' && *end_pointer != '\n') ||
		id > MAX_INT || id < -MAX_INT ) {
        printf("Error, please provide a valid integer. \n");
		return -1;
    }


	int amount = (int) id;
	printf("\n");
 	printf("Enter an amount to increase or decrease the counter \n");
	printf("Enter 0 to exit, a positive number to increase or a negative number to decrease \n");

	char user_amount[100];
	sprintf(user_amount, "%i", amount);

	send(sockD, user_amount, sizeof(amount), 0);

	if (amount == 0) {
		printf("Exiting... \n");
		return 0;
	}
	else
		return 1;
}

int give_exit_choice(int sockD){
	int choice;
	printf("Do you want to exit, then type '0', or continue, then type '1': \n");
	scanf("%1s", choice);

	while(!isdigit(choice)) // for security purposes to avoid unexpected behavior
	{
		printf("Please enter a valid choice: \n");
		scanf("%1s", choice);
	}

		if (choice == 0){
			// Send message to exit connection with server
			char message[255] = "exit";
			send(sockD, message, sizeof(message), 0);
			return 0;
	}
	return 1;
}


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
		printf("Connection error \n");
	}
	else { // Actions here

		char strData[255];

		// Initial (dummy) message
		recv(sockD, strData, sizeof(strData), 0);
		printf("%s\n", strData);

		int error = -1;
		while (error == -1) {
			error = send_ID(sockD);
		}	



		int server_id_answer;
		recv(sockD, &server_id_answer, sizeof(server_id_answer), 0);

		// if answer == 0 -> Id exists so login
		// if answer == 1 ->  Id does not exist so create new password

		if(server_id_answer == 0){
			int error;
			do {
				error = send_Password(sockD);
			} while (error == -1);
			error = 0;
			
			int server_pass_answer;
			recv(sockD, &server_pass_answer, sizeof(server_pass_answer), 0);
			while (server_pass_answer == 1) {
				printf("Wrong password, please try again \n");
				int error;
				do {
					error = send_Password(sockD);
				} while (error == -1);
				error = 0;

				recv(sockD, &server_pass_answer, sizeof(server_pass_answer), 0);
			}
		}
		else if(server_id_answer == 1){
			printf("This user is not yet known. A new account will be created. \n");
			int error;
			do {
				error = send_Password(sockD);
			} while (error == -1);

		}

		printf("login successful! \n");
		// By now, login is successful


		int counter;
		do {
			counter = give_counter_choice(sockD);
			sleep(3);
		} while (counter != 0);

		close(sockD);
	}

	return 0;
}


