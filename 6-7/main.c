#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define DB_SIZE 10
#define PRINT_DB 0
#define MIN_DB_VALUE 0
#define MAX_DB_VALUE 100
#define DB_SHARED_MEMORY_NAME "hse-os-ihw2-db-shared-memory"
#define SEMAPHORE_SHARED_MEMORY_NAME "hse-os-ihw2-semaphore-shared-memory"
#define PROCESSES_SLEEP_TIME 2

void sig_handler(int sig_num);
void reader_process();
void writer_process();
void print_db();
void sort_db();

int parent_pid;
int *db;
sem_t *db_write_sem_id;

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

  // Create and fill db
  int shm_db_fd;
  if ((shm_db_fd = shm_open(DB_SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666)) == -1) {
    perror("shm_open: failed to create shared memory object for db");
    exit(1);
  }
  if (ftruncate(shm_db_fd, DB_SIZE * sizeof(int)) == -1) {
    perror("ftruncate: failed to set the size of the shared memory object for db");
    exit(1);
  }
  db = (int *)mmap(NULL, DB_SIZE * sizeof(int), PROT_READ | PROT_WRITE,
                   MAP_SHARED, shm_db_fd, 0);
  for (int i = 0; i < DB_SIZE; ++i) {
    db[i] = rand() % (MAX_DB_VALUE - MIN_DB_VALUE + 1) + MIN_DB_VALUE;
  }
  sort_db();

  // Create semaphore using POSIX unnamed semaphore
  int shm_sem_fd;
  if ((shm_sem_fd = shm_open(SEMAPHORE_SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666)) == -1) {
    perror("shm_open: failed to create shared memory object for semaphore");
    exit(1);
  }
  if (ftruncate(shm_sem_fd, sizeof(sem_t)) == -1) {
    perror("ftruncate: failed to set the size of the shared memory object for semaphore");
    exit(1);
  }
  db_write_sem_id = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE,
                               MAP_SHARED, shm_sem_fd, 0);
  if (sem_init(db_write_sem_id, 1, 1) == -1) {
    perror("sem_init: failed to initialize semaphore");
    exit(1);
  }


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

  // Clear resources

  if (munmap(db, DB_SIZE * sizeof(int)) == -1) {
    perror("munmap: failed to unmap the shared memory object for db");
    exit(1);
  }
  if (close(shm_db_fd) == -1) {
    perror("close: failed to close the shared memory object for db");
    exit(1);
  }
  if (shm_unlink(DB_SHARED_MEMORY_NAME) == -1) {
    perror("shm_unlink: failed to remove the shared memory object for db");
    exit(1);
  }

  if (sem_destroy(db_write_sem_id) == -1) {
    perror("sem_destroy: failed to destroy semaphore");
    exit(1);
  }
  if (munmap(db_write_sem_id, sizeof(sem_t)) == -1) {
    perror("munmap: failed to unmap the shared memory object for semaphore");
    exit(1);
  }
  if (close(shm_sem_fd) == -1) {
    perror("close: failed to close the shared memory object for semaphore");
    exit(1);
  }
  if (shm_unlink(SEMAPHORE_SHARED_MEMORY_NAME) == -1) {
    perror("shm_unlink: failed to remove the shared memory object for semaphore");
    exit(1);
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
  exit(0);
}

void reader_process() {
  while (1) {
    int ind = rand() % DB_SIZE;
    int calc_val = db[ind] * (ind + 1);
    printf("[reader, %d] Value at index %d is %d, calculated value is %d\n",
           getpid(), ind, db[ind], calc_val);
    print_db();

    sleep(PROCESSES_SLEEP_TIME);
  }
}

void writer_process() {
  while (1) {
    sem_wait(db_write_sem_id);

    int ind = rand() % DB_SIZE;
    int new_val = rand() % (MAX_DB_VALUE - MIN_DB_VALUE + 1) + MIN_DB_VALUE;
    int prev_val = db[ind];
    db[ind] = new_val;
    printf("[writer, %d] Changed value at index %d from %d to %d\n", getpid(),
           ind, prev_val, new_val);
    sort_db();
    print_db();
    sleep(1);

    sem_post(db_write_sem_id);

    sleep(PROCESSES_SLEEP_TIME);
  }
}

void print_db() {
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

void sort_db() {
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
