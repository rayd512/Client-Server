// This code contains modified code from:
// binarytides.com/server-client-example-c-sockets-linux/
#include <iostream>
#include <algorithm>
#include <sstream>
#include <vector>
#include <utility>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

// Maximum number of connections
#define MAX_CON 1000

using namespace std;

// Forward declare functions
void check_input(int argc, char* argv[]);
void Trans( int n );
void log_output(string log_name, int trans_time, int trans_num, string mode);
void inc_tracker(vector<pair<string, int> > &client_tracker, string log_name);
void write_summary(vector<pair<string, int> > &client_tracker,
				   double start_time, double end_time, int trans_num);
double get_epoch_time();

int main(int argc, char *argv[]) {
	// Init variables
	int socket_desc, c, read_size, max_sd,
		monitor, trans_num = 0;
	struct sockaddr_in server, client;
	char client_message[1000], reply[1000];
	struct timeval timeout = {30, 0};
	fd_set sock_set;
	vector<pair<string, int> > client_tracker;
	double start_time = 0, end_time = 0;
	vector<int> client_sock;
	
	// Check validity of input passed from command line
	check_input(argc, argv);

	//Create socket
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1) {
		perror("Error: Could not create socket");
		exit(EXIT_FAILURE);
	}
	
	// Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(stoi(argv[1]));
	
	// Bind
	if( bind(socket_desc,(struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("Error: Bind Failed");
		exit(EXIT_FAILURE);
	}
	
	cout << "Using port " << stoi(argv[1]) << endl;

	// Listen
	if(listen(socket_desc, MAX_CON) < 0) {
		perror("Error: Listen Error");
		exit(EXIT_FAILURE);
	}
	c = sizeof(struct sockaddr_in);

	// Main loop of recieving clients and messages and sending messages
	while(true) {
		// Clear the set
		FD_ZERO(&sock_set);
		// Add socket_desc to the set
		FD_SET(socket_desc, &sock_set);
		// Hold max sd(int) for select function
		max_sd = socket_desc;

		// Add socket descriptors into the set
		for(int i = 0; i < (int)client_sock.size(); i++) {
			int new_sd = client_sock[i];
			FD_SET(new_sd, &sock_set);

			if(new_sd > max_sd) {
				max_sd = new_sd;
			}
		}

		// Use select to monitor activity from sockets. 
		monitor = select(max_sd + 1, &sock_set, NULL, NULL, &timeout); 

		if (monitor < 0) {
			cout << "Error: Select error" << endl;
			exit(EXIT_FAILURE);
		} else if (monitor == 0) {
			// Timeout has occured, exit the program
			fflush(stdout);
			close(socket_desc);
			write_summary(client_tracker, start_time, end_time, trans_num);
			exit(EXIT_SUCCESS);
		}

		// Check if data is avaible from socket_desc
		if (FD_ISSET(socket_desc, &sock_set)) {

			// Accept a new connection
			int new_sock = accept(socket_desc, (struct sockaddr *)&client,
								(socklen_t*)&c);

			if (new_sock < 0) {
				perror("Error: Accept Failed");
				exit(EXIT_FAILURE);
			}

			// Add new socket to vector
			if((int)client_sock.size() <= MAX_CON) {
				client_sock.push_back(new_sock);
			} else {
				cout << "Too many connections" << endl;
			}
		}
		
		// Check for available data on other sd's
		for(int i = 0; i < (int)client_sock.size(); i++) {
			// Get sd and check for data availability
			int sd = client_sock[i];
			if (!FD_ISSET(sd, &sock_set))
				continue;

			// Recieve message from client
			memset(client_message, 0, 1000);
			if ((read_size = recv(sd, client_message, 1000, 0)) > 0 ) {

				string transaction, log_name;

				// Split message recieved by whitespace
				string message = string(client_message);
				istringstream stream(message);
				stream >> transaction >> log_name;

				// Increment the counter for the specific client
				inc_tracker(client_tracker, log_name);

				// Extract the int 
				int trans_time = stoi(transaction.substr(1,
														transaction.size()-1));
				trans_num++;

				// Output to terminal before and after transaction
				log_output(log_name, trans_time, trans_num, "Trans");
				Trans(trans_time);
				log_output(log_name, trans_time, trans_num, "Done");

				// Get the time of the first transaction
				if(trans_num == 1) {
					start_time = get_epoch_time();
				}

				// Get the time of the last transaction
				end_time = get_epoch_time();

				// Create a reply
				sprintf(reply, "Done %d", trans_num);

				// Send message to client
				if(write(sd, reply, strlen(reply)) == -1) {
					perror("Error: Writing Failed");
					exit(EXIT_FAILURE);
				}
				
				// Reset arrays
				memset(client_message, 0, 1000);
				memset(reply, 0, 1000);
			}
			
			if(read_size == 0) {
				// Client has disconnected
				close(sd);
				// Remove from vector
				client_sock.erase(client_sock.begin()+i);
			}
			else if(read_size == -1) {
				perror("Error: recv failed");
			}
		}
	}
	return 0;
}

// Checks if the input from terminal meets the criteria
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

// Prints to stdout what the client is doing
void log_output(string log_name, int trans_time, int trans_num, string mode) {
	if(mode == "Done") {
		printf("%.2f: # %2d (Done) from %s\n", get_epoch_time(),
											trans_num, log_name.c_str());
	} else {
		printf("%.2f: # %2d (T%3d) from %s\n", get_epoch_time(),
									trans_num, trans_time, log_name.c_str());
	}
}

// Returns the epoch time
double get_epoch_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return (double) (tv.tv_sec) + (double) (tv.tv_usec) / 1000000;
}

// Increments the amount of transactions each connected client has requested
void inc_tracker(vector<pair<string, int> > &client_tracker, string log_name) {
	bool is_tracked = false;
	// Increment client's transaction amount if in vector
	for(int i = 0; i < (int)client_tracker.size(); i++) {
		if(client_tracker[i].first == log_name) {
			client_tracker[i].second++;
			is_tracked = true;
			break;
		}
	}
	// Add new client to vector
	if(!is_tracked) {
		pair <string, int> new_client;
		new_client.first = log_name;
		new_client.second = 1;
		client_tracker.push_back(new_client);
	}
}

// Write the summary when server ends
void write_summary(vector<pair<string, int> > &client_tracker,
				   double start_time, double end_time, int trans_num) {
	double elapsed_time;
	double trans_rate;

	cout << endl;
	cout << "SUMMARY" << endl;
	// Output client stats 
	for(int i = 0; i < (int)client_tracker.size(); i++) {
		printf("%4d transactions from %s\n", client_tracker[i].second,
											 client_tracker[i].first.c_str());
	}
	
	// Get the elapsed time. 
	if (end_time == start_time && start_time != 0) {
		// Case if only one transaction happens. Grab the current time
		elapsed_time = get_epoch_time() - (double)start_time;
		trans_rate = trans_num/elapsed_time;
	} else if (end_time == start_time) {
		elapsed_time = 0;
		trans_rate = 0;
	} else {
		elapsed_time = (double)end_time - (double)start_time;
		trans_rate = trans_num/elapsed_time;
	}

	// print the trans/sec
	printf("%4.1f transactions/sec (%d/%.2f) \n",
			trans_rate, trans_num, elapsed_time);
}