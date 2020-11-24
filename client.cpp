#include <iostream>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

using namespace std;

void check_input(int argc, char* argv[]);
void Sleep( int n );
void log_out(int int_time, string mode, FILE* fp);
void process_reply(string server_reply, FILE* fp, int num_trans);
void write_footer(FILE* fp, int num_trans);
double get_epoch_time();
string get_logname();

int main(int argc, char* argv[]) {
	
	int sock, num_trans = 0;
	struct sockaddr_in server;
	char message[1000], server_reply[1000];

	check_input(argc, argv);
	string log_name = get_logname();
	// cout << log_name << endl;
	//Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	
	if (sock == -1) {
		perror("Error: Could not create socket");
		exit(EXIT_FAILURE);
	}

	// cout << "Socket created" << endl;
	server.sin_addr.s_addr = inet_addr(argv[2]);
	server.sin_family = AF_INET;
	server.sin_port = htons(stoi(argv[1]));

	//Connect to remote server
	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("Error: Connection Failed");
		exit(EXIT_FAILURE);
	}
	
	// cout << "Connected" << endl;
	FILE* fp = fopen(log_name.c_str(), "w+");
	fprintf(fp, "Using port %s\n", argv[1]);
	fprintf(fp, "Using server address %s\n", argv[2]);
	fprintf(fp, "Host%s\n", log_name.c_str());

	while(true) {
		memset(message, 0, 1000);
		if (scanf("%s", message) != 1) {
			break;
		}
		
		if(message[0] == 'T') {
			int trans_time = stoi(string(message).substr(1,
									string(message).size()-1));

			strcat(message, log_name.c_str());

			//Send some data
			// puts("Server send :");
			// puts(message);
			log_out(trans_time, "Trans", fp);
			if (send(sock, message, strlen(message), 0) < 0) {
				puts("Send failed");
				return 1;
			}
			num_trans++;
			//Receive a reply from the server
			memset(server_reply, 0, 1000);
			if (recv(sock, server_reply, 1000, 0) < 0) {
				puts("recv failed");
				break;
			}
			// puts("Server reply: ");
			// puts(server_reply);
			process_reply(string(server_reply), fp, num_trans);
		} else {

			string sleep = string(message);
			int sleep_time = stoi(sleep.substr(1, sleep.size()-1));
			log_out(sleep_time, "Sleep", fp);
			if (sleep_time < 0 || sleep_time > 100) {
				cout << "Invalid sleep time" << endl;
				exit(EXIT_FAILURE);
			}

			Sleep(sleep_time);
		}
	}

	write_footer(fp, num_trans);
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

double get_epoch_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return (double) (tv.tv_sec) + (double) (tv.tv_usec) / 1000000;
}

void log_out(int int_time, string mode, FILE* fp) {
	if(mode == "Trans") {
		fprintf(fp, "%.2f: Send (T%3d)\n", get_epoch_time(), int_time);
	} else {
		fprintf(fp, "Sleep %d units\n", int_time);
	}
}

void write_footer(FILE* fp, int num_trans) {
	fprintf(fp, "Sent %d Transactions\n", num_trans);
	fclose(fp);
}

void process_reply(string server_reply, FILE* fp, int num_trans) {
	string message, trans_string;
	int trans_num;
	if(server_reply == "") {
		cout << "Error: Server is not available" << endl;
		write_footer(fp, num_trans);
		exit(EXIT_FAILURE);
	}

	istringstream stream(server_reply);

	stream >> message >> trans_string;

	if(message == "Done") {
		trans_num = stoi(trans_string);
		fprintf(fp, "%.2f: Recv (D%3d)\n", get_epoch_time(), trans_num);
	}

}