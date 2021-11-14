#include <pthread.h>
#include <stdlib.h>
#include "buffer.h"
/**
 * Author: Christopher Spartz
 * Utlizes Ring buffers to act as input and output queues
 * Ring buffer was used since it makes it easy to determine
 * where a new element should go, and the least recently used element
 */ 

/**
 * Initializes the buffers
 */ 
queue_t* createQueue(unsigned int capacity) {
	queue_t* Buffer = (queue_t*)malloc(sizeof(queue_t));
    if (Buffer == NULL) {
        return NULL;
    } else {
        Buffer->head.index = 0;
	    Buffer->tail.index = -1;
		pthread_mutex_init(&Buffer->head.lock,NULL);
		pthread_mutex_init(&Buffer->tail.lock,NULL);
	    Buffer->size = 0;
	    Buffer->capacity = capacity;
        Buffer->data = (int*)malloc(capacity * sizeof(int));
		Buffer->lock = (pthread_mutex_t*)malloc(capacity * sizeof(pthread_mutex_t));
        Buffer->state = (int*)malloc(capacity * sizeof(int));

        for (int i = 0; i < capacity; i++) {
            pthread_mutex_init(&Buffer->lock[i], NULL);
        }
        for (int i = 0; i < capacity; i++) {
            Buffer->state[i] = 0;
        }

        return (Buffer->data == NULL) ? NULL : Buffer;
    }
}

/**
 * Checks if the buffer is full
 */ 
int is_full(queue_t* buffer) {
	if(buffer->size == buffer->capacity) {
		return 1;
	}else {
		return 0;
	}
}

/**
 * Checks if the buffer is empty
 */ 
int is_empty(queue_t* buffer) {
	if(buffer->size == 0) {
		return 1;
	}else {
		return 0;
	}
}

/**
 * Adds to the buffer at the tail
 */ 
void enqueue(queue_t* buffer, int val) {
	if (is_full(buffer) == 1){
		return;
	} else {
		buffer->tail.index = (buffer->tail.index + 1) % buffer->capacity;
		buffer->data[buffer->tail.index] = val;
		buffer->size++;
	}
}

/**
 * Removes the head from the buffer
 */ 
int dequeue(queue_t* buffer) {
	if (is_empty(buffer) == 1){
		return 0;
	}else{
		 int item = buffer->data[buffer->head.index];
		 buffer->head.index = (buffer->head.index + 1) % buffer->capacity;
		 buffer->size--;
		 return item;
	}
}

/**
 * Frees all of the memory that was malloced in a buffer
 * Also destroys the locks in the buffers
 */ 
void free_memory(queue_t* buffer) {
	pthread_mutex_destroy(&buffer->head.lock);
	pthread_mutex_destroy(&buffer->tail.lock);
	for(int i=0;i<buffer->capacity;i++)
	{
		pthread_mutex_destroy(&buffer->lock[i]);
	}
	
	free(buffer->lock);
	free(buffer->data);
	free(buffer->state);

	free(buffer);
}