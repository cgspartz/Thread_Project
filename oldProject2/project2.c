#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <ctype.h>
#include <semaphore.h>
#include "encrypt-module.h"
#include "buffer.h"

int inBufferSize;
int outBufferSize;
queue_t* inputBuffer;
queue_t* outputBuffer;
//Array to help tell if previous threads are running
int threadControl[4];   
int reset;
sem_t resetS, emptyBuffers;
//Semaphores used to tell when the previous thread has completed its tasks
sem_t read, input, enc, output, fullInput, fullOutput; 

/**
 * Reads from the input file and places the char in the input buffer
 * Locks the tail and its position to ensure safety when adding a new char to the buffer
 */ 
void *readerThread()
{
    int c;
    unsigned int lockI;
    int oldTail;
    threadControl[0]=1;
    //pthread_cond_wait()
    while ((c = read_input()) != EOF) {
        // Block if queue if full
        if(is_full(inputBuffer)==1)
            sem_wait(&fullInput);
        pthread_mutex_lock(&inputBuffer->tail.lock);
        //Different case then normal at the beginning of the program
        if(inputBuffer->tail.index==-1){
            pthread_mutex_lock(&inputBuffer->lock[0]);
            enqueue(inputBuffer,c);
            pthread_mutex_unlock(&inputBuffer->tail.lock);
            pthread_mutex_unlock(&inputBuffer->lock[0]);
        }
        else{
            if(inputBuffer->tail.index==inputBuffer->capacity-1){
                lockI=0;
            }
            else {    
                lockI=inputBuffer->tail.index+1;
            }
            pthread_mutex_lock(&inputBuffer->lock[lockI]);
            enqueue(inputBuffer, c);
            //count_input(c);
            pthread_mutex_unlock(&inputBuffer->tail.lock);
            pthread_mutex_unlock(&inputBuffer->lock[inputBuffer->tail.index]);
        }
        sem_post(&read);
        if(reset==1)
            sem_wait(&resetS);
    }
    threadControl[0]=0;
    sem_post(&read);
    return NULL;
}

/**
 * Thread to count each new char in the input buffer
 * Locks the part of the buffer where it is changing the state
 * Loops through the buffer since it doesn't remove from the buffer
 */ 
void *inputCounterThread()
{
    int oldLock;
    int currentLocation = inputBuffer->head.index;
    //wait pid for reader thread
    //printf("HERE");
    threadControl[1]=1;
    while(1){
        //while(is_empty(inputBuffer) == 0){
            printf("%d",oldLock);
            oldLock++;
            sem_wait(&read);
            if(threadControl[0]==0) continue;
            pthread_mutex_lock(&inputBuffer->lock[currentLocation]);
            if(inputBuffer->state[currentLocation]==0){
                count_input(inputBuffer->data[currentLocation]);
                inputBuffer->state[currentLocation]=1;
            }
            pthread_mutex_unlock(&inputBuffer->lock[currentLocation]);
            pthread_mutex_lock(&inputBuffer->tail.lock);
            if(currentLocation==inputBuffer->tail.index)
            {  
                pthread_mutex_lock(&inputBuffer->head.lock);
                currentLocation=inputBuffer->head.index;
                pthread_mutex_unlock(&inputBuffer->head.lock);
            }
            else 
                currentLocation++;
            pthread_mutex_unlock(&inputBuffer->tail.lock);
            sem_post(&input);
        //}
    }
    threadControl[1]=0;
    sem_post(&input);
    return NULL;
}

void *inputCount()
{
    //int currentLocation = inputBuffer->head.index;
    threadControl[1]=1;
    while(1)
    {
        sem_wait(&read);
        if(threadControl[0]==0) break;
        count_input(inputBuffer->data[inputBuffer->head.index]);
        sem_post(&input);
        // if(inputBuffer->state[currentLocation]==0)
        // {
        //     count_input(inputBuffer->data[currentLocation]);
        //     inputBuffer->state[currentLocation]=1;
        //     sem_post(&input);
        // }
        // if(currentLocation==inputBuffer->tail.index)
        // {
        //     currentLocation=inputBuffer->head.index;
        // }
        // else{
        //     currentLocation++;
        // }
                
    }
    threadControl[1]=0;
    sem_post(&input);
}

/**
 * Thread that takes a variable from the input buffer
 * It encrypts it and then places that new character in the output buffer.
 * It locks the head and head's position in the input buffer
 * To make sure it is the only one messing with the head
 * It then locks the tail and its position
 * To make sure that outputcount and writethread are not in a race condition here
 */
void *encryptionThread()
{
    int c;
    unsigned int lockI;
    int oldLoc;

    threadControl[2]=1;
    //while inputcounter is running
    while(1){
        // while(is_empty(inputBuffer)==0)
        // {
            sem_wait(&input);
            if(threadControl[1]==0) break;
            //input buffer
            pthread_mutex_lock(&inputBuffer->head.lock);
            pthread_mutex_lock(&inputBuffer->lock[inputBuffer->head.index]);
            oldLoc = inputBuffer->head.index;
            c = caesar_encrypt(dequeue(inputBuffer));
            if(inputBuffer->size==inputBuffer->capacity-2)
                sem_post(&fullInput);
            pthread_mutex_unlock(&inputBuffer->head.lock);
            pthread_mutex_unlock(&inputBuffer->lock[oldLoc]);

            //output buffer
            if(is_full(outputBuffer)==1)
                sem_wait(&fullOutput);
            pthread_mutex_lock(&outputBuffer->tail.lock);
            //Different case then normal at the beginning of the program
            if(outputBuffer->tail.index==-1)
            {
                pthread_mutex_lock(&outputBuffer->lock[0]);
                enqueue(outputBuffer,c);
                pthread_mutex_unlock(&outputBuffer->tail.lock);
                pthread_mutex_unlock(&outputBuffer->lock[0]);
            }
            else{
                if(outputBuffer->tail.index==outputBuffer->capacity-1){
                    lockI=0;
                }
                else {    
                    lockI=outputBuffer->tail.index+1;
                }
                pthread_mutex_lock(&outputBuffer->lock[lockI]);
                enqueue(outputBuffer, c);
                //count_output(c);
                pthread_mutex_unlock(&outputBuffer->tail.lock);
                pthread_mutex_unlock(&outputBuffer->lock[outputBuffer->tail.index]);
            }
            sem_post(&enc);
        //}
    }
    threadControl[2]=0;
    sem_post(&enc);
    return NULL;
}

/**
 * Thread to count each new char in the output buffer
 * Locks the part of the buffer where it is changing the state
 * Loops through the buffer since it doesn't remove from the buffer
 */ 
void *outputCounterThread()
{
    int oldLock;
    int currentLocation = outputBuffer->head.index;
    int status;
    //wait pid for reader thread
    while(threadControl[2]==0);
    threadControl[3]=1;
    while(threadControl[2]==1){
    //while(is_empty(outputBuffer) == 0){
        sem_wait(&enc);
        if(threadControl[2]==0) continue;
        pthread_mutex_lock(&outputBuffer->lock[currentLocation]);
        if(outputBuffer->state[currentLocation]==0){
                count_input(outputBuffer->data[currentLocation]);
                outputBuffer->state[currentLocation]=1;       
        }
        pthread_mutex_unlock(&outputBuffer->lock[currentLocation]);
        pthread_mutex_lock(&outputBuffer->tail.lock);
        if(currentLocation==outputBuffer->tail.index)
        {  
            pthread_mutex_lock(&outputBuffer->head.lock);
            currentLocation=outputBuffer->head.index;
            pthread_mutex_unlock(&outputBuffer->head.lock);
        }
        else 
            currentLocation++;
        pthread_mutex_unlock(&outputBuffer->tail.lock);
        sem_post(&output);
    //}
}
    threadControl[3]=0;
    sem_post(&output);
    return NULL;
}

void *outputCount()
{
    int currentLocation = outputBuffer->head.index;
    threadControl[3]=1;
    while(1)
    {
        sem_wait(&enc);
        if(threadControl[2]==0) break;
        count_output(outputBuffer->data[inputBuffer->head.index]);
        sem_post(&output);
        // if(outputBuffer->state[currentLocation]==0)
        // {
        //     count_input(outputBuffer->data[currentLocation]);
        //     outputBuffer->state[currentLocation]=1;
        //     sem_post(&input);
        // }
        // if(currentLocation==outputBuffer->tail.index)
        // {
        //     currentLocation=outputBuffer->head.index;
        // }
        // else{
        //     currentLocation++;
        // }
                
    }
    threadControl[3]=0;
    sem_post(&output);
}

/**
 * This thread writes to the output file
 * It locks the head index of the output buffer
 * Also locks the lock in the heads position in the buffer
 * Locks both so that nothing besides this thread can modify it
 */ 
void *writerThread()
{
    int c;
    unsigned int lockI;
    int oldLoc;
    while(1){
        sem_wait(&output);
        if(threadControl[3]==0) break;
        pthread_mutex_lock(&outputBuffer->head.lock);
        pthread_mutex_lock(&outputBuffer->lock[outputBuffer->head.index]);
        oldLoc = outputBuffer->head.index;
        write_output(dequeue(outputBuffer));
        if(inputBuffer->size==inputBuffer->capacity-2)
            sem_post(&fullOutput);
        pthread_mutex_unlock(&outputBuffer->head.lock);
        pthread_mutex_unlock(&outputBuffer->lock[oldLoc]);
        if(reset==1 && is_empty(inputBuffer)==1 && is_empty(outputBuffer)==1)
            sem_post(&emptyBuffers);
    }
    return NULL;
}

/**
 * Displays the count of each char with the current key
 */ 
void display_counts()
{
    printf("Total input count with current key is %d \n",get_input_total_count());
    for( char l = 'a'; l<='z'; ++l) {
        printf("%c:%d ",toupper(l),get_input_count(l));
    }
    printf("\nTotal output count with current key is %d \n",get_output_total_count());
    for( char l = 'a'; l<='z'; ++l) {
        printf("%c:%d ",toupper(l),get_output_count(l));
    }
    printf("\n");
}

int main(int argc, char** argv)
{
    if(argc>3)
    {
        printf("Incorrect command: ./encrypt inputFile outputFile");
        exit(-1);
    }
    
    char* in; 
    char* out;
    in=argv[1];
    out=argv[2];
    char c;
    inBufferSize=0;
    outBufferSize=0;
    reset=0;
    sem_init(&read, 0, 0);
    sem_init(&input, 0, 0);
    sem_init(&enc, 0, 0);
    sem_init(&output, 0, 0);
    sem_init(&fullInput, 0, 0);
    sem_init(&fullOutput, 0, 0);
    sem_init(&resetS,0,0);
    sem_init(&emptyBuffers,0,0);
    init(in,out);
    // for(int i=0;i<4;i++)
    // {
    //     pthread_cond_init(&threadControl[i], NULL);
    // }

    printf("Enter input buffer size: ");
    scanf("%d",&inBufferSize);
    printf("Enter output buffer size: ");
    scanf("%d",&outBufferSize);
    //Checks to make sure buffers are valid sizes
    if(inBufferSize < 1 || outBufferSize < 1)
    {
        fprintf(stderr,"Input and output buffers must be >1\n");
        exit(-1);
    }
    //Initializes thread control
    for(int i=0;i<5;i++)
    {
        threadControl[i]=0;
    }
    
    pthread_t tid[5];
    inputBuffer = createQueue(inBufferSize);
    outputBuffer = createQueue(outBufferSize);
    pthread_create(&tid[0],NULL,&readerThread,NULL);
    pthread_create(&tid[1],NULL,&inputCount,NULL);
    pthread_create(&tid[2],NULL,&encryptionThread,NULL);
    pthread_create(&tid[3],NULL,&outputCount,NULL);
    pthread_create(&tid[4],NULL,&writerThread,NULL);
    // for(int i=0;i<5;i++)
    // {
    //     pthread_join(tid[i],NULL);
    // }
    pthread_join(tid[0],NULL);  //join the threads here
    pthread_join(tid[1],NULL);
    pthread_join(tid[2],NULL);
    pthread_join(tid[3],NULL);
    pthread_join(tid[4],NULL);

    printf("\nEnd of file reached.\n");
    display_counts();
    sem_destroy(&read);
    sem_destroy(&input);
    sem_destroy(&output);
    sem_destroy(&enc);
    // for(int i=0;i<4;i++)
    // {
    //     pthread_cond_destroy(&threadControl[i]);
    // }
    free_memory(inputBuffer);
    free_memory(outputBuffer);
}


/**
 * Sets reset to 1 to stop the read thread
 * Then waits until both buffers are empty to display counts
 */ 
void reset_requested()
{
    //usleep(10000);
    reset=1;
    sem_wait(&emptyBuffers);
    //while (is_empty(outputBuffer)==0 && is_empty(inputBuffer)==0);
    printf("\nReset requested.\n");
    display_counts();
}

/**
 * Sets reset back to 0 to let the read thread run again
 */ 
void reset_finished()
{
    reset=0;
    sem_post(&resetS);
    printf("Reset finished.\n");
}