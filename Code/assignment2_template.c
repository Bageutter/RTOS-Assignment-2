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
  pthread_create(&(tid[1]), &attr, &ThreadB, (void*)(&params));
  pthread_create(&(tid[2]), &attr, &ThreadC, (void*)(&params));
  
  // Wait on threads to finish
  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);
  pthread_join(tid[2], NULL);

  sem_destroy(&params.sem_A);
  sem_destroy(&params.sem_B);
  sem_destroy(&params.sem_C);

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

  // Create pipe for A->B communication
  if (pipe(params->pipeFile) == -1) {
    perror("pipe");
    exit(1);
  }

  // Initialize shared memory for B->C communication
  shm_fd = shm_open(SHARED_MEM_NAME, O_CREAT | O_RDWR, 0600);
  if (shm_fd == -1) {
    perror("shm_open");
    exit(1);
  }

  if (ftruncate(shm_fd, SHARED_MEM_SIZE) == -1) {
    perror("ftruncate");
    exit(1);
  }

  return;
}

void* ThreadA(void *params) {
  //TODO: add your code
  printf("\033[31mThread A: started and reading data from input\033[0m\n");
  ThreadParams *threadParams = (ThreadParams*) params; // Cast the void pointer to ThreadParams pointer
  sem_wait(&threadParams->sem_A); // Wait for semaphore A to be available

  FILE *dataInput = fopen(threadParams->inputFile, "r");
  if (dataInput == NULL) {
    perror("Error opening file");
    exit(1);
  }

  int sum = 0; // Initialize line counter
  char dataLine[255];
  while(fgets(dataLine, sizeof(dataLine), dataInput) != NULL) {
    sum++; // Increment sum for each line read
    // Remove trailing newline from fgets
    if (dataLine[strlen(dataLine)-1] == '\n') {
      dataLine[strlen(dataLine)-1] = '\0';
    }
    printf("\033[31mThread A: read line %d from file; data = %s\033[0m\n", sum, dataLine);

    write(threadParams->pipeFile[1], dataLine, strlen(dataLine)); // Write the data line to the pipe
    
    printf("\033[31mThread A: successfully wrote line to pipe: %s\033[0m\n", dataLine);
    
    sem_post(&threadParams->sem_B); // Signal that semaphore B can proceed    
    sem_wait(&threadParams->sem_A); // Wait for semaphore A to be available for the next line

    printf("\033[31mThread A: received signal from Thread C, ready to read next line from file...\033[0m\n");
  }
  close(threadParams->pipeFile[1]);
  fclose(dataInput);
  printf("\033[31mThread A: sum = %d\033[0m\n", sum);
  printf("\033[31mThread A: Closing\033[0m\n");
  sem_post(&threadParams->sem_B); // Signal B one last time for EOF
  return NULL;
}

void* ThreadB(void *params) {
  printf("\033[94mThread B: started and waiting for data from Thread A...\033[0m\n");
  ThreadParams *threadParams = (ThreadParams*) params; // Cast the void pointer to ThreadParams pointer

  // Map shared memory for writing data
  char *shm_ptr = mmap(NULL, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }

  char pipeData[255];
  int dataLength;
  int sum = 0; // Initialize sum
  
  while(1) {
    sem_wait(&threadParams->sem_B); // Wait for semaphore B to be available
    sum++; // Increment sum for each line processed
    // Try to read from A->B pipe
    dataLength = read(threadParams->pipeFile[0], pipeData, sizeof(pipeData)-1);
    
    if (dataLength <= 0) {
      // EOF reached
      printf("\033[94mThread B: EOF detected, signaling Thread C\033[0m\n");
      // Clear shared memory to signal EOF
      memset(shm_ptr, 0, SHARED_MEM_SIZE);
      // Signal Thread C one last time to process EOF
      sem_post(&threadParams->sem_C);
      break;
    }
    
    pipeData[dataLength] = '\0';
    printf("\033[94mThread B: received data from Thread A: %s\033[0m\n", pipeData);
    
    // Write data to shared memory
    strncpy(shm_ptr, pipeData, SHARED_MEM_SIZE - 1);
    shm_ptr[SHARED_MEM_SIZE - 1] = '\0';
    printf("\033[94mThread B: wrote data to shared memory\033[0m\n");
    
    // Signal Thread C to process the data
    sem_post(&threadParams->sem_C);
  }
  
  close(threadParams->pipeFile[0]);
  munmap(shm_ptr, SHARED_MEM_SIZE);
  printf("\033[94mThread B: sum = %d\033[0m\n", sum);
  printf("\033[94mThread B: Closing\033[0m\n");
  return NULL;
}

void* ThreadC(void *params) {
  //TODO: add your code
  printf("\033[33mThread C: started and waiting for data from Thread B...\033[0m\n");
  ThreadParams *threadParams = (ThreadParams*) params; // Cast the void pointer to ThreadParams pointer

  // Map shared memory for reading data
  char *shm_ptr = mmap(NULL, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }

  FILE *dataOutput = fopen(threadParams->outputFile, "w");
  if (dataOutput == NULL) {
    perror("Error opening output file");
    exit(1);
  }

  int headerFinished = 0; // Flag to track if we've passed the "end_header" line
  int sum = 0;
  int firstIteration = 1; // Flag for first iteration
  
  while(1) {
    sem_wait(&threadParams->sem_C); // Wait for semaphore C to be available
    // Check if EOF signal (empty shared memory after first iteration)
    if (strlen(shm_ptr) == 0 && !firstIteration) {
      // Empty shared memory after processing data means EOF
      printf("\033[33mThread C: EOF detected, closing...\033[0m\n");
      break;
    }
    firstIteration = 0;
    
    printf("\033[33mThread C: received data from Thread B: %s\033[0m\n", shm_ptr);
    
    // Check if this is the "end_header" line
    if (!headerFinished && (strcmp(shm_ptr, "end_header\n") == 0 || strcmp(shm_ptr, "end_header") == 0)) {
      printf("\033[33mThread C: Found 'end_header', will start writing content from next line\033[0m\n");
      headerFinished = 1;
    } else if (headerFinished) {
      // Write data to output file only if we've passed the header
      fprintf(dataOutput, "%s", shm_ptr);
      printf("\033[33mThread C: Wrote content line to output file\033[0m\n");
      sum++;
    } else {
      // Discard line otherwise
      printf("\033[33mThread C: Skipping header line\033[0m\n");
    }
    
    // Clear shared memory before signaling Thread A
    memset(shm_ptr, 0, SHARED_MEM_SIZE);
    
    // Signal Thread A that output is complete
    sem_post(&threadParams->sem_A);
  }
  
  printf("Thread C: Total content lines written: %d\n", sum);
  fclose(dataOutput);
  munmap(shm_ptr, SHARED_MEM_SIZE);
  shm_unlink(SHARED_MEM_NAME);
  printf("\033[33mThread C: Total content lines written (Final Sum): %d\033[0m\n", sum);
  printf("\033[33mThread C: Closing\033[0m\n");
  return NULL;
}
