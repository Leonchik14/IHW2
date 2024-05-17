#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "common.h"

void sig_handler(int sig_num);

int shm_id;
int *db;
int db_write_sem_id;

int main(int argc, char *argv[]) {
  // Register signal handler
  signal(SIGINT, sig_handler);

  // Create and fill db
  db = get_db(IPC_CREAT, &shm_id);
  for (int i = 0; i < DB_SIZE; ++i) {
    db[i] = get_random_db_value();
  }
  sort_db(db);
  printf("Successfully created and filled db\n");

  // Create and initialize semaphore
  db_write_sem_id = get_db_write_sem_id(IPC_CREAT);
  if (semctl(db_write_sem_id, 0, SETVAL, 1) == -1) {
    perror("semctl: failed to initialize the semaphore");
    exit(1);
  }
  printf("Successfully created and initialized semaphore\n");

  // Wait for signals
  while (1) {
  }

  return 0;
}

void sig_handler(int sig_num) {
  if (sig_num != SIGINT) {
    fprintf(stderr, "unexpected signal: %d\n", sig_num);
    exit(1);
  }

  // Remove used resources
  if (shmdt(db) == -1) {
    perror("shmdt: failed to detach the shared memory object");
    exit(1);
  }
  if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
    perror("shmctl: failed to remove the shared memory object");
    exit(1);
  }
  if (semctl(db_write_sem_id, 0, IPC_RMID) == -1) {
    perror("semctl: failed to remove the semaphore");
    exit(1);
  }
  printf("Successfully removed used resources\n");

  exit(0);
}
