#ifndef BUFFER_H
#define BUFFER_H
/**
 * Author: Christopher Spartz
 * Utlizes Ring buffers to act as input and output queues
 * Ring buffer was used since it makes it easy to determine
 * where a new element should go, and the least recently used element
 */ 

typedef struct node{
	unsigned int index;
	pthread_mutex_t lock;
} node;

struct queue {
	node tail;	    // current tail
	node head;	    // current head
	unsigned int size;	    // current number of items
	unsigned int capacity;      // Capacity of queue
    int* state;  //0=ready to count 1=ready to encrypt
	pthread_mutex_t* lock;
	int* data; 		    // Pointer to array of data
};

typedef struct queue queue_t;

queue_t* createQueue(unsigned int capacity);

int is_full(queue_t* buffer);

int is_empty(queue_t* buffer);

void enqueue(queue_t* buffer ,int val);

int dequeue(queue_t* buffer);

void free_memory(queue_t* buffer);

#endif