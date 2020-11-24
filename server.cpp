#include <iostream>
#include <sstream>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_CON 1000

using namespace std;

void check_input(int argc, char* argv[]);
void Trans( int n );
void log_output(string log_name, int trans_time, int trans_num);
double get_epoch_time();

int main(int argc, char *argv[]) {

	int socket_desc, client_sock[MAX_CON], c, read_size, max_sd,
		monitor, trans_num = 0;
	struct sockaddr_in server, client;
	char client_message[1000];
	struct timeval timeout = {30, 0};
	fd_set sock_set;

	check_input(argc, argv);

	for(int i = 0; i < MAX_CON; i++) {
		client_sock[i] = 0;
	}

	//Create socket
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	
	if (socket_desc == -1) {
		perror("Error: Could not create socket");
		exit(EXIT_FAILURE);
	}
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(stoi(argv[1]));
	
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("Error: Bind Failed");
		exit(EXIT_FAILURE);
	}
	
	cout << "Using port " << stoi(argv[1]) << endl;

	//Listen
	if(listen(socket_desc, MAX_CON) < 0) {
		perror("Error: Listen Error");
		exit(EXIT_FAILURE);
	}

	c = sizeof(struct sockaddr_in);

	while(true) {

		//Accept and incoming connection
		// cout << "Waiting for incoming connections..." << endl;
		
		FD_ZERO(&sock_set);
		FD_SET(socket_desc, &sock_set);
		max_sd = socket_desc;

		for(int i = 0; i < MAX_CON; i++) {
			int child_sd = client_sock[i];

			if(child_sd > 0) {
				FD_SET(child_sd, &sock_set);
			}

			if(child_sd > max_sd) {
				max_sd = child_sd;
			}
		}

		monitor = select(max_sd + 1, &sock_set, NULL, NULL, &timeout); 
		//accept connection from an incoming client
		
		if (monitor < 0) {
			cout << "Error: Select error" << endl;
		} else if (monitor == 0) {
			cout << "Timeout" << endl;
			close(socket_desc);
			exit(EXIT_SUCCESS);
		}

		if (FD_ISSET(socket_desc, &sock_set)) {

			int child_sock = accept(socket_desc, (struct sockaddr *)&client,
								(socklen_t*)&c);

			if (child_sock < 0) {
				perror("Error: Accept Failed");
				exit(EXIT_FAILURE);
			}

			// puts("Connection accepted");

			for(int i = 0; i < MAX_CON; i++) {
				if(client_sock[i] == 0) {
					client_sock[i] = child_sock;
					break;
				}
			}
		}
		
		
		for(int i = 0; i < MAX_CON; i++) {
			int sd = client_sock[i];

			if (!FD_ISSET(sd, &sock_set))
				continue;

			memset(client_message, 0, 1000);
			if ((read_size = recv(sd, client_message, 1000, 0)) > 0 ) {
				trans_num++;
				string transaction, log_name;
				string message = string(client_message);
				istringstream stream(message);

				stream >> transaction >> message;

				int trans_time = stoi(transaction.substr(1,
														transaction.size()-1));
				// cout << trans_time << endl;
				log_output(log_name, trans_time, trans_num);
				Trans(trans_time);
				//Send the message back to client
				printf( "Client recv: %s\n", client_message );
				if(write(sd, client_message, strlen(client_message)) == -1) {
					perror("Error: Writing Failed");
					exit(EXIT_FAILURE);
				}
				
				printf( "Client sent: %s\n", client_message );
				memset(client_message, 0, 1000);
			}
			
			if(read_size == 0) {
				// puts("Client disconnected");
				fflush(stdout);
				close(sd);
				client_sock[i] = 0;
			}
			else if(read_size == -1) {
				perror("recv failed");
			}
		}
	}
	return 0;
}


void check_input(int argc, char* argv[]) {
	if(argc > 2) {
		cout << "Error: Too many inputs" << endl;
		cout << "server <port>" << endl;
		exit(EXIT_FAILURE);
	} else if (argc == 1) {
		cout << "Error: Not enough inputs" << endl;
		cout << "server <port>" << endl;
		exit(EXIT_FAILURE);
	}

	if(stoi(argv[1]) < 5000 || stoi(argv[1]) > 64000) {
		cout << "Error: Invalid port number" << endl;
		cout << "Port number must be in the range 5000 to 64000 inclusive" 
			 << endl;
		exit(EXIT_FAILURE);
	}
}

void log_output(string log_name, int trans_time, int trans_num) {
	printf("%.2f\n", get_epoch_time());
}

double get_epoch_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);

	cout << tv.tv_usec << endl;
	return (double) (tv.tv_sec) + (double) (tv.tv_usec) / 1000000;
}