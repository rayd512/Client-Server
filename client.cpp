// This code contains modified code from:
// binarytides.com/server-client-example-c-sockets-linux/
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

// Forward declare functions
void check_input(int argc, char* argv[]);
void Sleep( int n );
void log_out(int int_time, string mode, FILE* fp);
void process_reply(string server_reply, FILE* fp, int num_trans);
void write_footer(FILE* fp, int num_trans);
double get_epoch_time();
string get_logname();

int main(int argc, char* argv[]) {
	// Init variables
	int sock, num_trans = 0;
	struct sockaddr_in server;
	char message[1000], server_reply[1000];

	// Check validity of input
	check_input(argc, argv);

	// Get the log file name
	string log_name = get_logname();

	// Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		perror("Error: Could not create socket");
		exit(EXIT_FAILURE);
	}

	// Setup server
	server.sin_addr.s_addr = inet_addr(argv[2]);
	server.sin_family = AF_INET;
	server.sin_port = htons(stoi(argv[1]));

	//Connect to remote server
	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("Error: Connection Failed");
		exit(EXIT_FAILURE);
	}
	
	// Open log file and write the header
	FILE* fp = fopen(log_name.c_str(), "w+");
	fprintf(fp, "Using port %s\n", argv[1]);
	fprintf(fp, "Using server address %s\n", argv[2]);
	fprintf(fp, "Host%s\n", log_name.c_str());

	// Main loop for message passing between client and server
	while(true) {
		// Get message
		memset(message, 0, 1000);
		if (scanf("%s", message) != 1) {
			break;
		}
		
		// Handle sending transactions
		if(message[0] == 'T') {
			// Extract the int 
			int trans_time = stoi(string(message).substr(1,
									string(message).size()-1));

			// Add the logfile name to the message
			strcat(message, log_name.c_str());

			// Add transaction to the log
			log_out(trans_time, "Trans", fp);
			
			// Send message to the server
			if (send(sock, message, strlen(message), 0) < 0) {
				puts("Send failed");
				exit(EXIT_FAILURE);
			}

			// Increment the number of transactions sent
			num_trans++;

			//Receive a reply from the server
			memset(server_reply, 0, 1000);
			if (recv(sock, server_reply, 1000, 0) < 0) {
				puts("Recv failed");
				break;
			}

			// Process the reply from the server
			process_reply(string(server_reply), fp, num_trans);
		} else {
			// Extract amount of units to sleep for
			string sleep = string(message);
			int sleep_time = stoi(sleep.substr(1, sleep.size()-1));

			// Check validity of sleep time
			if (sleep_time < 0 || sleep_time > 100) {
				cout << "Invalid sleep time" << endl;
				exit(EXIT_FAILURE);
			}

			// Log sleep
			log_out(sleep_time, "Sleep", fp);
			Sleep(sleep_time);
		}
	}

	// write the footer and close the socket
	write_footer(fp, num_trans);
	close(sock);
}

// Checks if the input parameters are correct according to the specifications
void check_input(int argc, char* argv[]) {
	// Check amount of inputs passed
	if(argc > 3) {
		cout << "Error: Too many inputs" << endl;
		cout << "client <port> <ip-address>" << endl;
		exit(EXIT_FAILURE);
	} else if (argc == 1) {
		cout << "Error: Not enough inputs" << endl;
		cout << "client <port> <ip-address>" << endl;
		exit(EXIT_FAILURE);
	}

	// Check is port number is valid
	if(stoi(argv[1]) < 5000 || stoi(argv[1]) > 64000) {
		cout << "Error: Invalid port number" << endl;
		cout << "Port number must be in the range 5000 to 64000 inclusive" 
			 << endl;
		exit(EXIT_FAILURE);
	}
}

// Gets the log file name
string get_logname() {
	char hostname[256];

	// Get the host name
	if(gethostname(hostname, sizeof(hostname)) == -1) {
		perror("Error: gethostname failure");
		exit(EXIT_FAILURE);
	}

	// reurn hostname and the process id number in a string
	return " " + string(hostname) + "." + to_string((int)getpid());
}

// Return the epoch time
double get_epoch_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return (double) (tv.tv_sec) + (double) (tv.tv_usec) / 1000000;
}

// Write to the log file
void log_out(int int_time, string mode, FILE* fp) {
	// Depending on the mode, write the correct output
	if(mode == "Trans") {
		fprintf(fp, "%.2f: Send (T%3d)\n", get_epoch_time(), int_time);
	} else {
		fprintf(fp, "Sleep %d units\n", int_time);
	}
}

// Write the summary of transactions
void write_footer(FILE* fp, int num_trans) {
	fprintf(fp, "Sent %d Transactions\n", num_trans);
	fclose(fp);
}

// Process the reply from the server
void process_reply(string server_reply, FILE* fp, int num_trans) {
	string message, trans_string;
	int trans_num;
	// If reply is blank, the server is not available. Close the 
	// footer and exit
	if(server_reply == "") {
		cout << "Error: Server is not available" << endl;
		write_footer(fp, num_trans);
		exit(EXIT_FAILURE);
	}

	// Split the output by space
	istringstream stream(server_reply);
	stream >> message >> trans_string;

	// Write to log file if message is correct
	if(message == "Done") {
		trans_num = stoi(trans_string);
		fprintf(fp, "%.2f: Recv (D%3d)\n", get_epoch_time(), trans_num);
	}

}