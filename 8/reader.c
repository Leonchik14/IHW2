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

int *db;

int main(int argc, char *argv[]) {
  // Register signal handler
  signal(SIGINT, sig_handler);

  // Get shared memory object and semaphore
  int shm_id;
  db = get_db(0, &shm_id);

  // Reader process
  while (1) {
    int ind = rand() % DB_SIZE;
    int calc_val = db[ind] * (ind + 1);
    printf("[reader, %d] Value at index %d is %d, calculated value is %d\n",
           getpid(), ind, db[ind], calc_val);
    print_db(db);

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
