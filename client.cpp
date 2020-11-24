#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

using namespace std;

void check_input(int argc, char* argv[]);
void Sleep( int n );
string get_logname();

int main(int argc, char* argv[]) {
	
	int sock;
	struct sockaddr_in server;
	char message[1000], server_reply[1000];

	check_input(argc, argv);
	string log_name = get_logname();
	cout << log_name << endl;
	//Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	
	if (sock == -1) {
		perror("Error: Could not create socket");
		exit(EXIT_FAILURE);
	}

	cout << "Socket created" << endl;
	server.sin_addr.s_addr = inet_addr(argv[2]);
	server.sin_family = AF_INET;
	server.sin_port = htons(stoi(argv[1]));

	//Connect to remote server
	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("Error: Connection Failed");
		exit(EXIT_FAILURE);
	}
	
	cout << "Connected" << endl;

	while(true) {
		memset(message, 0, 1000);
		printf("Enter message : ");
		if (scanf("%s", message) != 1) {
			break;
		}
		
		if(message[0] == 'T') {

			strcat(message, log_name.c_str());

			//Send some data
			puts("Server send :");
			puts(message);
			if (send(sock, message, strlen(message), 0) < 0) {
				puts("Send failed");
				return 1;
			}
			
			//Receive a reply from the server
			memset(server_reply, 0, 1000);
			if (recv(sock, server_reply, 1000, 0) < 0)
			{
				puts("recv failed");
				break;
			}
			puts("Server reply: ");
			puts(server_reply);
		} else {

			string sleep = string(message);
			int sleep_time = stoi(sleep.substr(1, sleep.size()-1));
			
			if (sleep_time < 0 || sleep_time > 100) {
				cout << "Invalid sleep time" << endl;
				exit(EXIT_FAILURE);
			}

			Sleep(sleep_time);
		}
	}

	close(sock);
}

void check_input(int argc, char* argv[]) {
	if(argc > 3) {
		cout << "Error: Too many inputs" << endl;
		cout << "client <port> <ip-address>" << endl;
		exit(EXIT_FAILURE);
	} else if (argc == 1) {
		cout << "Error: Not enough inputs" << endl;
		cout << "client <port> <ip-address>" << endl;
		exit(EXIT_FAILURE);
	}

	if(stoi(argv[1]) < 5000 || stoi(argv[1]) > 64000) {
		cout << "Error: Invalid port number" << endl;
		cout << "Port number must be in the range 5000 to 64000 inclusive" 
			 << endl;
		exit(EXIT_FAILURE);
	}
}

string get_logname() {
	char hostname[256];

	if(gethostname(hostname, sizeof(hostname)) == -1) {
		perror("Error: gethostname failure");
		exit(EXIT_FAILURE);
	}

	return " " + string(hostname) + "." + to_string((int)getpid());
}
