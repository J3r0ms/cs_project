#include <netinet/in.h> //structure for storing address information
#include <stdio.h> // for printf()
#include <stdlib.h> // for exit()
#include <sys/socket.h> //for socket APIs
#include <sys/types.h> //for data types
#include <ctype.h>
#include <string.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close


void send_ID(int sockD)
{
		int userID;

		printf("Enter user id: \n");
		scanf("%100i", &userID);

		char userInfo[100];

		sprintf(userInfo, "%i", userID);

		send(sockD, userInfo, sizeof(userInfo), 0);
}

void send_Password(int sockD)
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
 	printf("Enter an amount to increase or decrease the counter: \n");
	scanf("%d", &amount);
	send(sockD, &amount, sizeof(amount), 0);

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

		send_ID(sockD);

		int server_id_answer;
		recv(sockD, &server_id_answer, sizeof(server_id_answer), 0);

		// if answer == 0 -> Id exists so login
		// if answer == 1 ->  Id does not exist so create new password

		if(server_id_answer == 0){
			send_Password(sockD);
			
			int server_pass_answer;
			recv(sockD, &server_pass_answer, sizeof(server_pass_answer), 0);
			while (server_pass_answer == 1) {
				printf("Wrong password, please try again \n");
				send_Password(sockD);
				recv(sockD, &server_pass_answer, sizeof(server_pass_answer), 0);
			}
		}
		else if(server_id_answer == 1){
			printf("This user is not yet known. A new account will be created. \n");
			send_Password(sockD);

		}

		printf("login successful! \n");
		// By now, login is successful

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


