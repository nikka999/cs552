#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "server.h"

Params params;
int GloSocket = 0;
pthread_mutex_t conn_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t conn_cond = PTHREAD_COND_INITIALIZER;

void print_help(void) {
	printf("usage:	-h : print help message\n"
			"	-p : specify port number\n"
			"	-w : specify number of workers\n");	
}

int parse_args(int argc, char const **argv, Params *p) {
	int count = 1;
//initialize port and workers to default, incase user does not specify one or both of them.
	p->port = 5050;
	p->workers = 10;

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
			else if (!strcmp(argv[count], "-p")) {
				count++;
				p->port = atoi(argv[count]);
				count++;
			}
			else if (!strcmp(argv[count], "-w")) {
				count++;
				p->workers = atoi(argv[count]);
				count++;
			}
			else {
				printf("unkown parameter: %s\n", argv[count]);
				return 1;
			}
		}		
	}
	printf("Server started. Port: %d, Number of Workers: %d\n", p->port, p->workers);
	return 0;
}

void *do_work(void *thread_id) {
	int tid = (int)thread_id;
	int sd;
	char data[80];		/* Our receive data buffer. */  	
	pthread_mutex_lock(&conn_mutex);
	while (!GloSocket) {
		pthread_cond_wait(&conn_cond, &conn_mutex);
	}
	sd = GloSocket;
	GloSocket = 0;
	pthread_mutex_unlock(&conn_mutex);
	// while (1) {
		// write protocol, first send buffer size through port, then send string itself
		read (sd, &data, 14);
		printf ("Received string = %s, in thread %d\n", data, tid);
	// }
}
int init_cb(circular_buffer *cb, size_t sz) {
	cb->buffer = (char *) malloc(MAXSLOTS * sz);
	if (cb->buffer == NULL)
		return NO_MEMORY;
	cb->capacity = MAXSLOTS;
	cb->count = 0;
	cb->sz = sz;
	cb->buffer_end = (char *)cb->buffer + (MAXSLOTS * sz);
	cb->head = cb->buffer;
	cb->tail = cb->buffer;
	return SUCCESS;
}

int cb_push(circular_buffer *cb, const char *input) {
	if (cb->count == cb->capacity)
		return BUFFER_FULL;
	memcpy((void *)cb->head, (const void *)input, cb->sz);
	cb->head = cb->head + cb->sz;
	if (cb->head == cb->buffer_end)
        cb->head = cb->buffer;
    cb->count++;
	return SUCCESS;
}

int cb_pop(circular_buffer *cb, const char *output) {
	if (cb->count == 0)
		return BUFFER_EMPTY;
	memcpy((void *)output, (void *)cb->tail, cb->sz);
	cb->tail = cb->tail + cb->sz;
    if (cb->tail == cb->buffer_end)
        cb->tail = cb->buffer;
    cb->count--;
	return SUCCESS;
}

void free_cb(circular_buffer *cb) {
	free(cb->buffer);
}

int cb_count(circular_buffer *cb) {
	return cb->count;
}


void servConn (int port) {

  	int sd, new_sd;
  	struct sockaddr_in name, cli_name;
  	int sock_opt_val = 1;
  	int cli_len;
  	// char data[80];		/* Our receive data buffer. */
  	pthread_t threads[params.workers];
	int i, rc;
	// Create Worker Pool
	for (i = 0; i < params.workers; i++) {
		rc = pthread_create(&threads[i], NULL, do_work, (void *)i);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}	
	}
  	if ((sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
    	perror("(servConn): socket() error");
    	exit (-1);
  	}

  	if (setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, (char *) &sock_opt_val,
		  	sizeof(sock_opt_val)) < 0) {
    	perror ("(servConn): Failed to set SO_REUSEADDR on INET socket");
    	exit (-1);
  	}

  	name.sin_family = AF_INET;
  	name.sin_port = htons (port);
  	name.sin_addr.s_addr = htonl(INADDR_ANY);
  
  	if (bind (sd, (struct sockaddr *)&name, sizeof(name)) < 0) {
    	perror ("(servConn): bind() error");
    	exit (-1);
  	}

  	listen (sd, 5);

  	for (;;) {
      	cli_len = sizeof (cli_name);
      	new_sd = accept (sd, (struct sockaddr *) &cli_name, &cli_len);
      	printf ("Assigning new socket descriptor:  %d\n", new_sd);
      
      	if (new_sd < 0) {
			perror ("(servConn): accept() error");
			exit (-1);
      	}
		
		pthread_mutex_lock(&conn_mutex);
		GloSocket = new_sd;
		pthread_cond_signal(&conn_cond);
		pthread_mutex_unlock(&conn_mutex);
		
			//       	if (fork () == 0) {	/* Child process. */
			// close (sd);
			// read (new_sd, &data, 14); /* Read our string: "Hello, World!" */
			// printf ("Received string = %s\n", data);
			// exit (0);
			//       	}
			
  	}
}


int main (int argc, char const *argv[])
{
	int rc;
	
	rc = parse_args(argc, argv, &params);
	if (rc)
		exit(0);
	servConn (params.port);		/* Server port. */
	return 0;
}
