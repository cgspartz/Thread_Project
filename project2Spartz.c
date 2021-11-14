#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <ctype.h>
#include <semaphore.h>
#include "encrypt-module.h"

//Global variables to track the max capacity of both buffers
int inBufferSize;
int outBufferSize;

//Global variable int arrays to act as buffers to move chars between threads
int* inputBuffer;
int* outputBuffer;

//Description of the semaphores is done in main thread
sem_t readEmpty, fullInput, encFull, encEmpty, fullOutput, writeFull;
sem_t* inLocks;
sem_t* outLocks;

/**
 * Reads from the input file and places the char in the input buffer
 */ 
void *readerThread()
{
    int c;
    int location=0;
    while (1) {
        c = read_input();
        // Block if queue if full
        sem_wait(&readEmpty);
        //Blocks other threads from modifying this location in input buffer
        sem_wait(&inLocks[location]);
        inputBuffer[location]=c;
        sem_post(&inLocks[location]);
        sem_post(&encFull);
        sem_post(&fullInput);
        //If the chracter is EOF it ends the loop
        if(c==EOF)  
            break;
        //Increments location
        //Modulo to have it loop back on itself when it hits the size of the buffers
        location=(location+1)%inBufferSize;
    }
    return NULL;
}

/**
 * Thread to count each new char in the input buffer
 */
void *inputCount()
{
    int c, location=0;
    while(1)
    {
        sem_wait(&fullInput);
        //Blocks other threads from modifying this location in input buffer
        sem_wait(&inLocks[location]);
        c=inputBuffer[location];
        if(c!=EOF) 
            count_input(c);
        sem_post(&inLocks[location]);
        sem_post(&readEmpty);
        //Increments location
        //Modulo to have it loop back on itself when it hits the size of the buffers
        location=(location+1)%inBufferSize;
        //If the chracter is EOF it ends the loop
        if(c==EOF)  
            break;
    }
    return NULL;
}

/**
 * Thread that takes a variable from the input buffer
 * It encrypts it and then places that new character in the output buffer.
 */
void *encryptionThread()
{
    int c;
    int location=0, outLocation=0;
    while(1){
        //Input buffer section of encryption thread
        sem_wait(&encFull);
        //Blocks other threads from modifying this location in input buffer
        sem_wait(&inLocks[location]);
        c = inputBuffer[location];
        sem_post(&inLocks[location]);
        sem_post(&readEmpty);
        c=caesar_encrypt(c);
        //Output buffer section of the encryption thread
        sem_wait(&encEmpty);
        //Blocks other threads from modifying this location in output buffer
        sem_wait(&outLocks[outLocation]);
        outputBuffer[outLocation]=c;
        sem_post(&outLocks[outLocation]);
        sem_post(&fullOutput);
        sem_post(&writeFull);
        //Increments location and outlocation
        //Modulo to have it loop back on itself when it hits the size of the buffers
        location=(location+1)%inBufferSize;
        outLocation=(outLocation+1)%outBufferSize;
        //If the chracter is EOF it ends the loop
        if(c==EOF)  
            break;
    }
    return NULL;
}

/**
 * Thread to count each new char in the output buffer
 */ 
void *outputCount()
{
    int c, location=0;
    while(1)
    {
        sem_wait(&fullOutput);
        //Blocks other threads from modifying this location in output buffer
        sem_wait(&outLocks[location]);
        c=outputBuffer[location];
        if(c!=EOF) 
            count_output(c);
        sem_post(&outLocks[location]);
        sem_post(&encEmpty);
        //Increments location
        //Modulo to have it loop back on itself when it hits the size of the buffers
        location=(location+1)%outBufferSize;
        //If the chracter is EOF it ends the loop
        if(c==EOF)  break;
    }
    return NULL;
}

/**
 * This thread writes to the output file
 */ 
void *writerThread()
{
    int c;
    int location=0;
    while(1){
        sem_wait(&writeFull);
        //Blocks other threads from modifying this location in output buffer
        sem_wait(&outLocks[location]);
        c=outputBuffer[location];
        if(c!=EOF)  
            write_output(c);
        sem_post(&outLocks[location]);
        sem_post(&encEmpty);
        //Increments location
        //Modulo to have it loop back on itself when it hits the size of the buffers
        location=(location+1)%outBufferSize;
        //If the chracter is EOF it ends the loop
        if(c==EOF)  
            break;
    }
    return NULL;
}

/**
 * Displays the count of each char with the current key
 */ 
void display_counts()
{
    printf("Total input count with current key is %d \n",get_input_total_count());
    for(char l = 'a'; l<='z'; ++l) {
        printf("%c:%d ",toupper(l),get_input_count(l));
    }
    printf("\nTotal output count with current key is %d \n",get_output_total_count());
    for(char l = 'a'; l<='z'; ++l) {
        printf("%c:%d ",toupper(l),get_output_count(l));
    }
    printf("\n");
}

int main(int argc, char** argv)
{
    //If the arguments are not correct it notifies the user
    if(argc!=3)
    {
        printf("Incorrect command: ./encrypt inputFile outputFile");
        exit(-1);
    }
    
    char* in; 
    char* out;
    in=argv[1];
    out=argv[2];
    inBufferSize=0;
    outBufferSize=0;
    init(in,out);

    //Prompts the user for input and output buffer sizes
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

    //Input and output locks for each spot of the buffer to ensure concurrency
    inLocks = malloc(inBufferSize*sizeof(sem_t));
    outLocks = malloc(outBufferSize*sizeof(sem_t));

    //Initialize the semaphores at each part of the buffers
    //They're initialized to 1 to begin with so that it starts
    for(int i=0;i<inBufferSize;i++)
    {
        sem_init(&inLocks[i], 0, 1);
    }
    for(int i=0;i<outBufferSize;i++)
    {
        sem_init(&outLocks[i], 0, 1);
    }

    //These two semaphores are initialized to the
    //Sizes of the buffers in order to keep track of when the buffers are empty
    sem_init(&readEmpty, 0, inBufferSize);
    sem_init(&encEmpty, 0, outBufferSize);

    //The rest of the semaphores are to track if that thread has no more work to do
    sem_init(&encFull, 0, 0);
    sem_init(&writeFull, 0, 0);
    sem_init(&fullOutput, 0, 0);
    sem_init(&fullInput,0,0);
    
    pthread_t tid[5];
    //Malloc the char array buffers
    inputBuffer = malloc((inBufferSize+1)*sizeof(int));
    outputBuffer = malloc((outBufferSize+1)*sizeof(int));

    //Start every thread
    pthread_create(&tid[0],NULL,&readerThread,NULL);
    pthread_create(&tid[1],NULL,&inputCount,NULL);
    pthread_create(&tid[2],NULL,&encryptionThread,NULL);
    pthread_create(&tid[3],NULL,&outputCount,NULL);
    pthread_create(&tid[4],NULL,&writerThread,NULL);
    
    pthread_join(tid[0],NULL);  //join the threads here
    pthread_join(tid[1],NULL);
    pthread_join(tid[2],NULL);
    pthread_join(tid[3],NULL);
    pthread_join(tid[4],NULL);

    printf("\nEnd of file reached.\n");
    display_counts();

    //Destroy every semaphore created
    sem_destroy(&readEmpty);
    sem_destroy(&encEmpty);
    sem_destroy(&encFull);
    sem_destroy(&fullOutput);
    for(int i=0;i<inBufferSize;i++)
    {
        sem_destroy(&inLocks[i]);
    }
    for(int i=0;i<outBufferSize;i++)
    {
        sem_destroy(&outLocks[i]);
    }

    //Free the input and output buffers and locks
    free(inputBuffer);
    free(inLocks);
    free(outputBuffer);
    free(outLocks);
}

/**
 * Waits on each input semaphore to complete
 * Once each part of the buffer has had sem_wait called on it then it displays the counts.
 */ 
void reset_requested()
{
    for(int i=0;i<inBufferSize;i++)
    {
        sem_wait(&inLocks[i]);
    }
    printf("\nReset requested.\n");
    display_counts();
}

/**
 * Posts the input buffer semaphores at each location of the array
 */ 
void reset_finished()
{
    for(int i=0;i<inBufferSize;i++)
    {
        sem_post(&inLocks[i]);
    }
    printf("Reset finished.\n");
}