#include <netinet/in.h> //structure for storing address information
#include <stdio.h> // for printf()
#include <stdlib.h> // for exit()
#include <sys/socket.h> //for socket APIs
#include <sys/types.h> //for data types
#include <ctype.h>
#include <string.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close


void retrieve_ID(int sockD)
{
		int userID;

		printf("Enter user id: \n");
		scanf("%100i", &userID);

		char userInfo[100];

		sprintf(userInfo, "%i", userID);

		send(sockD, userInfo, sizeof(userInfo), 0);
}

void retrieve_Password(int sockD)
{
		char userPass[200];
		printf("Enter user password: \n");
		scanf("%200s", userPass);

		send(sockD, userPass, sizeof(userPass), 0);
		printf("Message sent \n");
}


void give_counter_choice(int sockD)
{
	int amount;
 	// Ask the user to type an id
 	printf("Enter a positive number to increase the counter or a "
	"negative counter to decrease the counter: \n");
 	// Get and save the number the user types
	scanf("%d", &amount);

	while(!isdigit(amount)) // for security purposes to avoid unexpected behavior
	{
		printf("Please enter a valid choice: \n");
		scanf("%d", &amount);
	}

	// if choice is increase
	if (amount >= 0){
		// Send message + amount to server
		char message[255] = "increase";
		send(sockD, message, sizeof(message), 0);
		send(sockD, &amount, sizeof(amount), 0);
	}

	// if choice is decrease
	else if (amount < 0){
		// Send message + amount to server
		char message[255] = "decrease";
		send(sockD, message, sizeof(message), 0);
		send(sockD, &amount, sizeof(amount), 0);
	}
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

		recv(sockD, strData, sizeof(strData), 0);
		printf("%s\n", strData);

		retrieve_ID(sockD);

		int server_id_answer;
		recv(sockD, &server_id_answer, sizeof(server_id_answer), 0);
		while (server_id_answer == 1) {
			printf("There was an error with your id, please try again \n");
			retrieve_ID(sockD);
		}
		
		retrieve_Password(sockD);

		int server_pass_answer;
		recv(sockD, &server_pass_answer, sizeof(server_pass_answer), 0);
		while (server_pass_answer == 1) {
			printf("There was an error with your password, please try again \n");
			retrieve_Password(sockD);
		}

		give_counter_choice(sockD);

		// Give the user a choice to exit or continue
		while(give_exit_choice(sockD) == 1){
			give_counter_choice(sockD);
		}

		// Close the socket
		close(sockD);
	}

	return 0;
}


