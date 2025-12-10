
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

struct message {
	char source[50];
	char target[50]; 
	char msg[200]; // message body
};

void terminate(int sig) {
	printf("Exiting....\n");
	fflush(stdout);
	exit(0);
}

int main() {
	int server;
	int target;
	int dummyfd;
	struct message req;
	signal(SIGPIPE,SIG_IGN);
	signal(SIGINT,terminate);
	server = open("serverFIFO",O_RDONLY);
	dummyfd = open("serverFIFO",O_WRONLY);

	while (1) {
		// TODO:
		// read requests from serverFIFO

		// Read the entire message structure from the server's FIFO.
		// read() is blocking until a message arrives.
		ssize_t bytes_read = read(server, &req, sizeof(struct message));

		if (bytes_read == 0) {
			close(server);
			server = open("serverFIFO", O_RDONLY);
			if (server == -1) {
				break;
			}//end error if
			continue;
		} else if (bytes_read == -1) {
			break;
		} else if (bytes_read != sizeof(struct message)) {
			continue;
		}//end error ifs

		printf("Received a request from %s to send the message %s to %s.\n",req.source,req.msg,req.target);

		// TODO:
		// open target FIFO and write the whole message struct to the target FIFO
		// close target FIFO after writing the message
		target = open(req.target, O_WRONLY);

		if (target == -1) {
			continue; 
		}//end check

		if (write(target, &req, sizeof(struct message)) == -1) {
		}//end check

		close(target);

	}
	close(server);
	close(dummyfd);
	return 0;
}

