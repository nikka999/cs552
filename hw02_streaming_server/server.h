#define MAXSLOTS 100

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

enum Errors {
	SUCCESS,
	NO_MEMORY,
	BUFFER_FULL,
	BUFFER_EMPTY,
};


void servConn (int port);
int init_cb(circular_buffer *cb, size_t sz);
int cb_push(circular_buffer *cb, const char *input);
int cb_pop(circular_buffer *cb, const char *output);
void free_cb(circular_buffer *cb);