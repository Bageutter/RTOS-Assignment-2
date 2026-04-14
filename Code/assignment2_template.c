/***********************************************************************************/
//***********************************************************************************
//            *************NOTE**************
// This is a template for the subject of RTOS in University of Technology Sydney(UTS)
// Please complete the code based on the assignment requirement.

//***********************************************************************************
/***********************************************************************************/

/*
  To compile assign2_template-v3.c ensure that gcc is installed and run 
  the following command:

  gcc your_program.c -o your_ass-2 -lpthread -lrt -Wall
*/

#include  <pthread.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <stdio.h>
#include  <sys/types.h>
#include  <fcntl.h>
#include  <string.h>
#include  <sys/stat.h>
#include  <semaphore.h>
#include  <sys/time.h>
#include  <sys/mman.h> 

/* to be used for your memory allocation, write/read. man mmsp */
#define SHARED_MEM_NAME "/my_shared_memory"
#define SHARED_MEM_SIZE 1024

/* --- Structs --- */
typedef struct ThreadParams {
  int pipeFile[2]; // [0] for read and [1] for write. use pipe for data transfer from thread A to thread B
  sem_t sem_A, sem_B, sem_C; // the semphore
  char message[255];
  char inputFile[100]; // input file name
  char outputFile[100]; // output file name
} ThreadParams;

pthread_attr_t attr;

int shm_fd;// use shared memory for data transfer from thread B to Thread C 

/* --- Prototypes --- */

/* Initializes data and utilities used in thread params */
void initializeData(ThreadParams *params, const char *inputFile, const char *outputFile);

/* This thread reads data from data.txt and writes each line to a pipe */
void* ThreadA(void *params);

/* This thread reads data from pipe used in ThreadA and writes it to a shared variable */
void* ThreadB(void *params);

/* This thread reads from shared variable and outputs non-header text to src.txt */
void* ThreadC(void *params);

/* --- Main Code --- */
int main(int argc, char const *argv[]) {
  
 pthread_t tid[3]; // three threads
 ThreadParams params;
    
    if (argc < 3) {
        fprintf(stderr, "Please give input & output file names\n e.g. %s input.txt output.txt\n", argv[0]);
        return 1;
    }

  // Initialization
  initializeData(&params, argv[1], argv[2]);

  // Create Threads
  pthread_create(&(tid[0]), &attr, &ThreadA, (void*)(&params));
  
  //TODO: add your code
  pthread_create(&(tid[1]), &attr, &ThreadB, (void*)(&params));
  //pthread_create(&(tid[2]), &attr, &ThreadC, (void*)(&params));
  
  //TODO: add your code
  // Wait on threads to finish
  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);
  //pthread_join(tid[2], NULL);

  sem_destroy(&params.sem_A);
  sem_destroy(&params.sem_B);
  //sem_destroy(&params.sem_C);

  return 0;
}

void initializeData(ThreadParams *params, const char *inputFile, const char *outputFile) {
  // Initialize Sempahores
  if(sem_init(&(params->sem_A), 0, 1) != 0) { // Set up Sem for thread A
    perror("error for init threa A");
    exit(1);
  }
if(sem_init(&(params->sem_B), 0, 0) != 0) { // Set up Sem for thread B
    perror("error for init threa B");
    exit(1);
  }
  if(sem_init(&(params->sem_C), 0, 0) != 0) { // Set up Sem for thread C
    perror("error for init threa C");
    exit(1);
  } 

// Initialize thread attributes 
  pthread_attr_init(&attr);
  //TODO: add your code

  strncpy(params->inputFile, inputFile, sizeof(params->inputFile) - 1); //These checks ensure that the input and output file names are properly copied into the params struct without exceeding the buffer size. The last character is set to '\0' to ensure null-termination of the strings.
  params->inputFile[sizeof(params->inputFile) - 1] = '\0';

  strncpy(params->outputFile, outputFile, sizeof(params->outputFile) - 1);
  params->outputFile[sizeof(params->outputFile) - 1] = '\0';

  if (pipe(params->pipeFile) == -1) {
    perror("pipe");
    exit(1);
  }

  return;
}

void* ThreadA(void *params) {
  //TODO: add your code
  int sum = 1;
  printf("Thread A: started and reading data from input\n");
  ThreadParams *threadParams = (ThreadParams*) params; // Cast the void pointer to ThreadParams pointer
  sem_wait(&threadParams->sem_A); // Wait for semaphore A to be available

  FILE *dataInput = fopen(threadParams->inputFile, "r");
  if (dataInput == NULL) {
    perror("Error opening file");
    exit(1);
  }

  int line= 1;
  char dataLine[255];
  while(fgets(dataLine, sizeof(dataLine), dataInput) != NULL) {
    printf("Thread A: read line %d from file; data = %s", sum, dataLine);

    write(threadParams->pipeFile[1], dataLine, strlen(dataLine)); // Write the data line to the pipe
    
    printf("Thread A: successfully wrote line to pipe: %s", dataLine);
    
    sem_post(&threadParams->sem_B); // Signal that semaphore B can proceed    
    sem_wait(&threadParams->sem_A); // Wait for semaphore A to be available for the next line

    printf("Thread A: received signal from Thread B, ready to read next line from file...\n");
  }
  close(threadParams->pipeFile[1]);
  fclose(dataInput);
  printf("Thread A: Closing\n");
  sem_post(&threadParams->sem_B);

  printf("Thread A: sum = %d\n", sum);
  return NULL;
}

void* ThreadB(void *params) {
  //TODO: add your code 
  int sum = 2;
  printf("Thread B: started and waiting for data from Thread A...\n");
  ThreadParams *threadParams = (ThreadParams*) params; // Cast the void pointer to ThreadParams pointer

  sem_wait(&threadParams->sem_B); // Wait for semaphore B to be available
  printf("Thread B: received signal from Thread A, reading data from pipe...\n");
  
  char pipeData[255];
  int n;
  
  while((n = read(threadParams->pipeFile[0], pipeData, sizeof(pipeData)-1)) > 0)
  {
    if (n > 0) {
        pipeData[n] = '\0';
        printf("Thread B: Successfully Read pipeData = %s\n", pipeData);
    }
    sem_post(&threadParams->sem_A); // Signal that semaphore A can proceed for the next line
    sem_wait(&threadParams->sem_B); // Wait for semaphore B to be available for the next line

    printf("Thread B: received signal from Thread A, reading data from pipe...\n");
  }
  
  close(threadParams->pipeFile[0]);
  printf("Thread B: sum = %d\n", sum);
  printf("Thread B: Closing\n");
  return NULL;
}

void* ThreadC(void *params) {
  //TODO: add your code
  int sum = 3; // Initialize sum variable

 printf("Thread C: Final sum = %d\n", sum);
 return NULL;
}
