#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

// Constants
const int DB_SIZE = 10;
const char *KEY_FILE_PATH = "..";
const char SEM_PROJ_ID = 's';
const char SHM_PROJ_ID = 'm';
const int MAX_DB_VALUE = 100;
const int MIN_DB_VALUE = 0;
const int PRINT_DB = 1;
const int PROCESSES_SLEEP_TIME = 2;

// Functions

int *get_db(int additional_flags, int *shm_id_ptr) {
  key_t shm_key = ftok(KEY_FILE_PATH, SHM_PROJ_ID);
  if (shm_key == -1) {
    perror("ftok: failed to generate a key for the shared memory object");
    exit(1);
  }
  *shm_id_ptr = shmget(shm_key, DB_SIZE * sizeof(int), additional_flags | 0666);
  if (*shm_id_ptr == -1) {
    perror("shmget: failed to create a shared memory object");
    exit(1);
  }
  int *db = (int *)shmat(*shm_id_ptr, NULL, 0);
  if (db == (int *)-1) {
    perror("shmat: failed to attach the shared memory object");
    exit(1);
  }
  return db;
}

int get_db_write_sem_id(int additional_flags) {
  key_t sem_key = ftok(KEY_FILE_PATH, SEM_PROJ_ID);
  if (sem_key == -1) {
    perror("ftok: failed to generate a key for the semaphore");
    exit(1);
  }
  int sem_id = semget(sem_key, 1, additional_flags | 0666);
  if (sem_id == -1) {
    perror("semget: failed to create a semaphore");
    exit(1);
  }
  return sem_id;
}

int get_random_db_value() {
  return MIN_DB_VALUE + rand() % (MAX_DB_VALUE - MIN_DB_VALUE + 1);
}

void sort_db(int *db) {
  for (int i = 0; i < DB_SIZE - 1; ++i) {
    for (int j = 0; j < DB_SIZE - i - 1; ++j) {
      if (db[j] > db[j + 1]) {
        int tmp = db[j];
        db[j] = db[j + 1];
        db[j + 1] = tmp;
      }
    }
  }
}

void print_db(int *db) {
  if (PRINT_DB == 0) {
    return;
  }

  printf("db = [");
  for (int i = 0; i < DB_SIZE; ++i) {
    printf("%d", db[i]);
    if (i < DB_SIZE - 1) {
      printf(", ");
    }
  }
  printf("]\n");
}
