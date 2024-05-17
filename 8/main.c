#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"

void sig_handler(int sig_num);
void reader_process();
void writer_process();

int parent_pid;
int *db;
int db_write_sem_id;

int main(int argc, char *argv[]) {
  // Register signal handler
  parent_pid = getpid();
  signal(SIGINT, sig_handler);

  // Handle command line arguments
  if (argc != 3) {
    fprintf(stderr, "usage: %s <reader-processes-num> <writer-processes-num>\n",
            argv[0]);
    exit(1);
  }
  char *endptr;
  long reader_processes_num = strtol(argv[1], &endptr, 10);
  if (*endptr != '\0') {
    fprintf(stderr,
            "invalid 1st argument: can't convert string '%s' to integer\n",
            argv[1]);
    exit(1);
  }
  long writer_processes_num = strtol(argv[2], &endptr, 10);
  if (*endptr != '\0') {
    fprintf(stderr,
            "invalid 2nd argument: can't convert string '%s' to integer\n",
            argv[2]);
    exit(1);
  }

  // Get shared memory object and semaphore
  int shm_id;
  db = get_db(0, &shm_id);
  db_write_sem_id = get_db_write_sem_id(0);

  // Create reader and writer processes
  setpgid(0, 0);
  for (long i = 0; i < reader_processes_num + writer_processes_num; ++i) {
    pid_t pid = fork();
    if (pid == -1) {
      perror("fork: failed to create a child process");
      exit(1);
    }
    if (pid == 0) {
      srand(getpid());

      if (i < reader_processes_num) {
        reader_process();
      } else {
        writer_process();
      }

      exit(0);
    }
  }

  // Wait for all child processes to finish
  for (long i = 0; i < reader_processes_num + writer_processes_num; ++i) {
    wait(NULL);
  }

  return 0;
}

void sig_handler(int sig_num) {
  if (sig_num != SIGINT) {
    fprintf(stderr, "unexpected signal: %d\n", sig_num);
    exit(1);
  }

  // Return to clear resources
  if (getpid() == parent_pid) {
    return;
  }

  // Detach shared memory object
  if (shmdt(db) == -1) {
    perror("shmdt: failed to detach the shared memory object");
    exit(1);
  }

  exit(0);
}

void reader_process() {
  while (1) {
    int ind = rand() % DB_SIZE;
    int calc_val = db[ind] * (ind + 1);
    printf("[reader, %d] Value at index %d is %d, calculated value is %d\n",
           getpid(), ind, db[ind], calc_val);
    print_db(db);

    sleep(PROCESSES_SLEEP_TIME);
  }
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

void writer_process() {
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

    sleep(1);

    unlock_sem();

    sleep(PROCESSES_SLEEP_TIME);
  }
}


