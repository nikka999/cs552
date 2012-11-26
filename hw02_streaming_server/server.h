#define MAXSLOTS 100
#define START_DISPATCH 1
#define MAX_PRIORITY 10
#define MESSAGE_SIZE 50
typedef struct circular_buffer
{
    char *buffer;     // data buffer
    char *buffer_end; // end of data buffer
    size_t capacity;  // maximum number of items in the buffer
    size_t count;     // number of items in the buffer
    size_t sz;        // size of each item in the buffer
    char *head;       // pointer to head
    char *tail;       // pointer to tail
} circular_buffer;

typedef struct worker_message
{
	int thread_id; // worker thread ID
	int fd; // Client Socket ID
	char message[MESSAGE_SIZE]; //Client Message
} worker_message;

enum Errors {
	SUCCESS,
	NO_MEMORY,
	BUFFER_FULL,
	BUFFER_EMPTY,
};

typedef struct{
	int port;
	int workers;
} Params;

void servConn (int port);
int init_cb(circular_buffer *cb, size_t sz);
int cb_push(circular_buffer *cb, const void *input);
int cb_pop(circular_buffer *cb, const void *output);
void free_cb(circular_buffer *cb);
int cb_count(circular_buffer *cb);
int parse_args(int argc, char const **argv, Params *p);
void *do_work(void *thread_id);
void *dispatcher(void *thread_id);
void *overflow_work(void *thread_id);
void intHandler(int sig);
int compare_messages(const void *a, const void *b);
int wm_2_int(const void *a);