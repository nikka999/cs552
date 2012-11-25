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
#include <signal.h>
#include "server.h"
#include <netpbm/ppm.h>

Params params;
int GloSocket = 0;
circular_buffer GloBuff;
int ActiveThreads = 0;
pthread_mutex_t conn_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buff_mutex = PTHREAD_MUTEX_INITIALIZER;
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
	int sd, rc;
	size_t buf_size = 0;
	char *data;		/* Our receive data buffer. */
	worker_message wm;
	while (1) {
		pthread_mutex_lock(&conn_mutex);
		while (!GloSocket) {
			pthread_cond_wait(&conn_cond, &conn_mutex);
		}
		sd = GloSocket;
		GloSocket = 0;
		ActiveThreads++;
		pthread_mutex_unlock(&conn_mutex);
		while (read(sd, &buf_size, sizeof(size_t)) > 0) {
			// write protocol, first send buffer size through port, then send string itself
			buf_size = ntohl(buf_size);
			data = (char *)malloc(buf_size+1);
			memset(data, 0, buf_size+1);
			rc  = read (sd, data, buf_size);
			if (rc != buf_size)
				printf("rc not right: %d\n", rc);
			printf ("Received string = %s, size is %lu, in thread %d\n", data, buf_size, tid);
			wm.thread_id = tid;
			wm.fd = sd;
			memset(wm.message, 0, MESSAGE_SIZE);
			strncpy(wm.message, data, MESSAGE_SIZE);
			while(cb_push(&GloBuff, &wm) == BUFFER_FULL);
			free(data);
		}
		printf("Client Disconnected\n");
		pthread_mutex_lock(&conn_mutex);
		ActiveThreads--;
		pthread_mutex_unlock(&conn_mutex);
		//close sd might not be good idea since old sd can be reused...
		close(sd);
	}
}

int wm_2_int(const void *a){
	const worker_message *wm = a;
	char *br1, *br2, *rank;
	int irank, i;
	br1 = strchr(wm->message, ':');
	br2 = strchr(br1+1, ':');
	irank = br2 - br1 - 1;
	rank = (char *)malloc(irank);
	br1++;
	for (i = 0; i < irank; i++)
		rank[i] = *(br1+i);
	rank[i] = '\0';
	irank = atoi(rank);
	free(rank);
	return irank;
}

int compare_messages(const void *a, const void *b) {
	int ia, ib;
	ia = wm_2_int(a);
	ib = wm_2_int(b);
	return ib - ia;
}

int get_arg(char msg[]) {
    int delim = ':';
    char * ptr;
    ptr = strrchr(msg, delim);
    //printf("%s, len=%d\n", ptr, strlen(ptr));
    char t[(strlen(ptr))];
	// get rid of first char=":"
    strncpy(t, (ptr + 1), (strlen(ptr)-1));
    t[strlen(ptr)-1] = '\0';
    int result = atoi(t);
    return result;
}

void *dispatcher(void *thread_id){
	worker_message wm[START_DISPATCH], buff_wm;
	char msg[40];
	size_t size = 0;
	int i, priority;
	while(1) {
		while(cb_count(&GloBuff) < START_DISPATCH);
		pthread_mutex_lock(&buff_mutex);
		for (i = 0; cb_pop(&GloBuff, &wm[i]) != BUFFER_EMPTY; i++);
		qsort(wm, START_DISPATCH, sizeof(worker_message), compare_messages);
		for (i = 0; i < START_DISPATCH; i++) {
			sprintf(msg, "%d,%d,%s", wm[i].thread_id, wm[i].fd, wm[i].message);
			printf("dispatcher: msg is %s\n", msg);
			
            /** SENDING IMAGE */
            pixel** pixarray;
            FILE *fp;
            int cols, rows;
            pixval maxval;
            unsigned char *buf;
            int frame_num = get_arg(msg);
            int x, y;
            char location[50];
            sprintf(location, "%s%d%s", "support/images/sw", frame_num, ".ppm");

            printf("%s\n", location);
            if ((fp = fopen (location,"r")) == NULL) {
                fprintf (stderr, "Can't open input file\n");
                exit (1);
            }
            pixarray = ppm_readppm (fp, &cols, &rows, &maxval);
            buf = (unsigned char *)malloc (cols*rows*3);
            printf("COLS = %d, ROWS = %d\n", cols, rows);
            for (y = 0; y < rows; y++) {
                for (x = 0; x < cols; x++) {
                    buf[(y*cols+x)*3+0] = PPM_GETR(pixarray[rows-y-1][x]);
                    buf[(y*cols+x)*3+1] = PPM_GETG(pixarray[rows-y-1][x]);
                    buf[(y*cols+x)*3+2] = PPM_GETB(pixarray[rows-y-1][x]);
                }
            }
		ppm_freearray(pixarray, rows);

            // Send type
            size_t len;
            len = 1;
            len = htonl(len);
            write(wm[i].fd, &len, sizeof(size_t));
            
            // Send size
            len = (cols*rows*3);
            len = htonl(len);
            write(wm[i].fd, &len, sizeof(size_t));
            
            // Send image
            write(wm[i].fd, buf, cols*rows*3);
            
			//write(wm[i].fd, msg, strlen(msg));
		}
		pthread_mutex_unlock(&buff_mutex);
	}
}

void *overflow_work(void *thread_id){
	int tid = (int)thread_id;
	int sd, rc;
	size_t buf_size = 0;
	char *data;		/* Our receive data buffer. */
	worker_message wm;
	while (1) {
		pthread_mutex_lock(&conn_mutex);
		if (GloSocket) {
			sd = GloSocket;
			GloSocket = 0;
		}
		else
			pthread_exit(NULL);
		pthread_mutex_unlock(&conn_mutex);
		while (read(sd, &buf_size, sizeof(size_t)) > 0) {
			// write protocol, first send buffer size through port, then send string itself
			buf_size = ntohl(buf_size);
			data = (char *)malloc(buf_size);
			rc  = read (sd, data, buf_size);
			if (rc != buf_size)
				printf("rc not right: %d\n", rc);
			printf ("Received string = %s, size is %lu, in thread %d\n", data, buf_size, tid);
			wm.thread_id = tid;
			wm.fd = sd;
			strncpy(wm.message, data, MESSAGE_SIZE);
			while(cb_push(&GloBuff, &wm) == BUFFER_FULL);
			free(data);
			memset(data, '\0', buf_size);
		}
		printf("Client Disconnected\n");
		//close sd might not be good idea since old sd can be reused...
		close(sd);
		pthread_exit(NULL);
	}
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

int cb_push(circular_buffer *cb, const void *input) {
	pthread_mutex_lock(&buff_mutex);
	if (cb->count == cb->capacity){
		pthread_mutex_unlock(&buff_mutex);
		return BUFFER_FULL;
	}
	memcpy((void *)cb->head, input, cb->sz);
	cb->head = cb->head + cb->sz;
	if (cb->head == cb->buffer_end)
        cb->head = cb->buffer;
    cb->count++;
	pthread_mutex_unlock(&buff_mutex);
	return SUCCESS;
}

int cb_pop(circular_buffer *cb, const void *output) {
	// pthread_mutex_lock(&buff_mutex);
	if (cb->count == 0) {
		// pthread_mutex_unlock(&buff_mutex);
		return BUFFER_EMPTY;
	}
	memcpy((void *)output, (void *)cb->tail, cb->sz);
	cb->tail = cb->tail + cb->sz;
    if (cb->tail == cb->buffer_end)
        cb->tail = cb->buffer;
    cb->count--;
	// pthread_mutex_unlock(&buff_mutex);
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
  	pthread_t threads[params.workers], disp;
	int i, rc;
	// Create Worker Pool
	for (i = 0; i < params.workers; i++) {
		rc = pthread_create(&threads[i], NULL, do_work, (void *)i);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}
	rc = pthread_create(&disp, NULL, dispatcher, (void *)i);
	if (rc) {
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
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
		printf("This is active threads: %d\n", ActiveThreads);
		if (ActiveThreads < params.workers)
			rc = pthread_cond_signal(&conn_cond);
		else {
			pthread_t over_flow;
			rc = pthread_create(&over_flow, NULL, overflow_work, (void *)++i);
			if (rc) {
				printf("ERROR; return code from pthread_create() is %d\n", rc);
			}
		}
		pthread_mutex_unlock(&conn_mutex);
  	}
}

void intHandler(int sig) {
	free_cb(&GloBuff);
	exit(0);
}

int main (int argc, char const *argv[])
{
	int rc;
	rc = parse_args(argc, argv, &params);
	if (rc) exit(0);
	rc = init_cb(&GloBuff, sizeof(worker_message));
	signal(SIGINT, intHandler);
   	signal(SIGKILL, intHandler);
	if (rc) exit(-1);
	servConn (params.port);		/* Server port. */
	return 0;
}
