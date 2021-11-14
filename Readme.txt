Project2
Written by Christopher Spartz

Project2Spartz.c: This file contains the entirety of my program.
It contains all of the threads, as it initializes them and waits for them to finish.
It contains the semaphores and buffers I needed for my threads to ensure concurrency and no race conditions.
Additionally it implements the reset requested and the reset finished methods from encrypt-module.h

The other text files in here are for testing, and if put through encrypt-module-reproducible.c with the make test
command it should always produce the same result.

This program was built to run on a linux thread and semaphore environment and will require changes to work on
mac or windows.