#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct{
	char* clientid;
	int priority;
} Params;

Params params;

void print_help(void) {
	printf("usage:	-h : print help message\n"
			"	-i : specify Client ID (in form number-hostname)\n"
			"	-p : specify Client Priority\n");	
}

int parse_args(int argc, char const **argv, Params *p) {
	int count = 1;
//initialize port and workers to default, incase user does not specify one or both of them.
	p->clientid = "1-nikka";
	p->priority = 10;

	if (argc == 1) {
		printf("You have not specified any parameters. So we will run using default params.\n");
		print_help();
	}
	else {
		while (count < argc) {
			if (!strcmp(argv[count], "-h")) {
				print_help();
				return 1;			
			}
			else if (!strcmp(argv[count], "-i")) {
				count++;
				p->clientid = (char *)argv[count];
				count++;
			}
			else if (!strcmp(argv[count], "-p")) {
				count++;
				p->priority = atoi(argv[count]);
				count++;
			}
			else {
				printf("unkown parameter: %s\n", argv[count]);
				return 1;
			}
		}		
	}
	printf("Client started. ID: %s, Priority: %d\n", p->clientid, p->priority);
	return 0;
}
void *recv_listen(void *sd) {
	int fd = (int) sd;
	size_t data_len;
	char *data;

	while(1) {
		read(fd, &data_len, sizeof(size_t));
		data = (char *)malloc(data_len);
		read(fd, data, data_len);
		printf("data is: %s\n",data);
	}	
}

int cliConn (char *host, int port) {
 
  struct sockaddr_in name;
  struct hostent *hent;
  int sd;
 
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("(cliConn): socket() error");
    exit (-1);
  }
 
  if ((hent = gethostbyname (host)) == NULL)
    fprintf (stderr, "Host %s not found.\n", host);
  else
    bcopy (hent->h_addr, &name.sin_addr, hent->h_length);
 
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
 
  /* connect port */
  if (connect (sd, (struct sockaddr *)&name, sizeof(name)) < 0) {
    perror("(cliConn): connect() error");
    exit (-1);
  }
 
  return (sd);
}

int command_line(int sd) {
	char buffer[20], msg[50];
	char *ptr, *args[5];
	int i;
	size_t len;
	memset(msg, '\0', 50);
	while (1) {
		while(fgets(buffer, 50, stdin)) {
			for (i = 0; buffer[i] != '\n'; i++);
			buffer[i] = '\0';
			i = 0;
			ptr = strtok(buffer, " ");
			while (ptr != NULL) {
				args[i] = ptr;
				ptr = strtok(NULL, " ");
				i++;				
			}
			if (!strcmp(args[0], "s")) {
				if (!strcmp(args[1], "start")) {
					if (i == 4)
						sprintf(msg, "%s:%d:%s:%s:%s", params.clientid, params.priority, "start_movie", args[2], args[3]);
				}
				else if (!strcmp(args[1], "seek")) {
					if (i == 4)
						sprintf(msg, "%s:%d:%s:%s:%s", params.clientid, params.priority, "seek_movie", args[2], args[3]);										
				}
				else if (!strcmp(args[1], "stop")) {
					if (i == 3)
						sprintf(msg, "%s:%d:%s:%s", params.clientid, params.priority, "stop_movie", args[2]);										
				}
				if (*msg != '\0') {
					len = strlen(msg);
					len = htonl(len);
					write(sd, &len, sizeof(size_t));
					printf("the msg is: %s\n", msg);
					write(sd, msg, strlen(msg));
					memset(msg, 0, 50);										
				}
			}
			else if(!strcmp(args[0], "q")) {
				// close(sd);
				return 1;
			}
		}
	}
}

int main (int argc, char const *argv[])
{
	int rc;
	// char buffer[20], msg[50];
	pthread_t listener;
	char *req = "stop_movie:batman";
	rc = parse_args(argc, argv, &params);
	if (rc) exit(0);		
  	int sd = cliConn ("localhost", 5050);
	rc = pthread_create(&listener, NULL, recv_listen, (void *) sd);
	if (rc) {
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}
	rc = command_line(sd);
	if (rc) exit(0);		
  	return 0;
}


