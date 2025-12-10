#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#define N 13

extern char **environ;
char uName[20];

char *allowed[N] = {"cp","touch","mkdir","ls","pwd","cat","grep","chmod","diff","cd","exit","help","sendmsg"};

struct message {
	char source[50];
	char target[50]; 
	char msg[200];
};

void terminate(int sig) {
        printf("Exiting....\n");
        fflush(stdout);
        exit(0);
}

void sendmsg (char *user, char *target, char *msg) {
	// TODO:
	// Send a request to the server to send the message (msg) to the target user (target)
	// by creating the message structure and writing it to server's FIFO
	int server_fd;
    struct message req;
    
    strncpy(req.source, user, sizeof(req.source) - 1);
    req.source[sizeof(req.source) - 1] = '\0';
    
    strncpy(req.target, target, sizeof(req.target) - 1);
    req.target[sizeof(req.target) - 1] = '\0';
    
    strncpy(req.msg, msg, sizeof(req.msg) - 1);
    req.msg[sizeof(req.msg) - 1] = '\0';
    
    server_fd = open("serverFIFO", O_WRONLY);
    
    if (server_fd == -1) {
        perror("rsh: Failed to open serverFIFO.");
        return;
    }//end fail open if
    
    ssize_t bytes_written = write(server_fd, &req, sizeof(struct message));
    
    if (bytes_written == -1) {
        perror("rsh: Failed to write to serverFIFO");	
    }//end fail write if  
    
    close(server_fd);
}//end sendmsg

void* messageListener(void *arg) {
	// TODO:
	// Read user's own FIFO in an infinite loop for incoming messages
	// The logic is similar to a server listening to requests
	// print the incoming message to the standard output in the
	// following format
	// Incoming message from [source]: [message]
	// put an end of line at the end of the message
	int user_fd;
    int temp_writer_fd;
    struct message incoming;
    
    if (mkfifo(uName, 0666) == -1) {
        if (errno != EEXIST) {
            perror("listener: Failed to create user FIFO");
            pthread_exit((void*)1); 
        }//end FIFO exist check if
    }//end user mkfifo if
    
    user_fd = open(uName, O_RDONLY);
    if (user_fd == -1) {
        perror("listener: Failed to open user FIFO for reading");
        pthread_exit((void*)1); 
    }//end read failure if
    
    temp_writer_fd = open(uName, O_WRONLY);
    if (dummy_writer_fd == -1) {
        perror("listener: Failed to open user FIFO for dummy writing");
        close(user_fd);
        pthread_exit((void*)1);
    }//end write failure if
    
    fprintf(stderr, "rsh: Listening for messages on FIFO '%s'...\n", uName);
    
    while (1) {
        ssize_t bytes_read = read(user_fd, &incoming, sizeof(struct message));
        fprintf(stdout, "\nIncoming message from [%s]: %s\nrsh>", incoming.source, incoming.msg);
        fflush(stdout); 
    }//end message while
    
    close(user_fd);
    close(dummy_writer_fd);
	pthread_exit((void*)0);
}//end messageListener

int isAllowed(const char*cmd) {
	int i;
	for (i=0;i<N;i++) {
		if (strcmp(cmd,allowed[i])==0) {
			return 1;
		}
	}
	return 0;
}

int main(int argc, char **argv) {
    pid_t pid;
    char **cargv; 
    char *path;
    char line[256];
    int status;
    posix_spawnattr_t attr;

    if (argc!=2) {
	printf("Usage: ./rsh <username>\n");
	exit(1);
    }
    signal(SIGINT,terminate);

    strcpy(uName,argv[1]);

    // TODO:
    // create the message listener thread
	pthread_t listener_thread;
	if (pthread_create(&listener_thread, NULL, messageListener, NULL) != 0) {
		perror("pthread_create failed");
		exit(EXIT_FAILURE);
	}//end create if
	
    while (1) {

	fprintf(stderr,"rsh>");

	if (fgets(line,256,stdin)==NULL) continue;

	if (strcmp(line,"\n")==0) continue;

	line[strlen(line)-1]='\0';

	char cmd[256];
	char line2[256];
	strcpy(line2,line);
	strcpy(cmd,strtok(line," "));

	if (!isAllowed(cmd)) {
		printf("NOT ALLOWED!\n");
		continue;
	}

	if (strcmp(cmd,"sendmsg")==0) {
		// TODO: Create the target user and
		// the message string and call the sendmsg function

		// NOTE: The message itself can contain spaces
		// If the user types: "sendmsg user1 hello there"
		// target should be "user1" 
		// and the message should be "hello there"

		// if no argument is specified, you should print the following
		// printf("sendmsg: you have to specify target user\n");
		// if no message is specified, you should print the followingA
 		// printf("sendmsg: you have to enter a message\n");

		char *target = strtok(NULL, " ");

		if (target == NULL) {
			printf("sendmsg: you have to specify target user\n");
			continue;
		}//end notarget if
		
		char *start_target = strstr(line2, target); 
		
		char *start_msg = start_target + strlen(target);
		
		while (*start_msg == ' ' && *start_msg != '\0') {
			start_msg++;
		}//end cleanup while
		
		if (*start_msg == '\0') {
			printf("sendmsg: you have to enter a message\n");
			//end nomessage if
		} else {
			sendmsg(uName, target, start_msg);
		}//end message if
		
		continue;
	}

	if (strcmp(cmd,"exit")==0) break;

	if (strcmp(cmd,"cd")==0) {
		char *targetDir=strtok(NULL," ");
		if (strtok(NULL," ")!=NULL) {
			printf("-rsh: cd: too many arguments\n");
		}
		else {
			chdir(targetDir);
		}
		continue;
	}

	if (strcmp(cmd,"help")==0) {
		printf("The allowed commands are:\n");
		for (int i=0;i<N;i++) {
			printf("%d: %s\n",i+1,allowed[i]);
		}
		continue;
	}

	cargv = (char**)malloc(sizeof(char*));
	cargv[0] = (char *)malloc(strlen(cmd)+1);
	path = (char *)malloc(9+strlen(cmd)+1);
	strcpy(path,cmd);
	strcpy(cargv[0],cmd);

	char *attrToken = strtok(line2," "); /* skip cargv[0] which is completed already */
	attrToken = strtok(NULL, " ");
	int n = 1;
	while (attrToken!=NULL) {
		n++;
		cargv = (char**)realloc(cargv,sizeof(char*)*n);
		cargv[n-1] = (char *)malloc(strlen(attrToken)+1);
		strcpy(cargv[n-1],attrToken);
		attrToken = strtok(NULL, " ");
	}
	cargv = (char**)realloc(cargv,sizeof(char*)*(n+1));
	cargv[n] = NULL;

	// Initialize spawn attributes
	posix_spawnattr_init(&attr);

	// Spawn a new process
	if (posix_spawnp(&pid, path, NULL, &attr, cargv, environ) != 0) {
		perror("spawn failed");
		exit(EXIT_FAILURE);
	}

	// Wait for the spawned process to terminate
	if (waitpid(pid, &status, 0) == -1) {
		perror("waitpid failed");
		exit(EXIT_FAILURE);
	}

	// Destroy spawn attributes
	posix_spawnattr_destroy(&attr);

    }
    return 0;
}
