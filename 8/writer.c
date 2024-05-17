#include "common.h"
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

void sig_handler(int sig_num);
void lock_sem();
void unlock_sem();

int *db;
int db_write_sem_id;

int main(int argc, char *argv[]) {
  // Register signal handler
  signal(SIGINT, sig_handler);

  // Get shared memory object and semaphore
  int shm_id;
  db = get_db(0, &shm_id);
  db_write_sem_id = get_db_write_sem_id(0);

  // Writer process
  while (1) {
    lock_sem();

    int ind = rand() % DB_SIZE;
    int new_val = get_random_db_value();
    int prev_val = db[ind];
    db[ind] = new_val;
    printf("[writer, %d] Changed value at index %d from %d to %d\n", getpid(),
           ind, prev_val, new_val);
    sort_db(db);
    print_db(db);
    
    unlock_sem();

    sleep(1);


    sleep(PROCESSES_SLEEP_TIME);
  }

  return 0;
}

void sig_handler(int sig_num) {
  if (sig_num != SIGINT) {
    fprintf(stderr, "unexpected signal: %d\n", sig_num);
    exit(1);
  }

  // Detach shared memory object
  if (shmdt(db) == -1) {
    perror("shmdt: failed to detach the shared memory object");
    exit(1);
  }

  exit(0);
}

void lock_sem() {
  struct sembuf sem_op = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};
  if (semop(db_write_sem_id, &sem_op, 1) < 0) {
    printf("semop: failed to lock semaphore\n");
    exit(-1);
  }
}

void unlock_sem() {
  struct sembuf sem_op = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};
  if (semop(db_write_sem_id, &sem_op, 1) < 0) {
    printf("semop: failed to unlock semaphore\n");
    exit(-1);
  }
}
