// Constants
extern const int DB_SIZE;
extern const char *KEY_FILE_PATH;
extern const char SEM_PROJ_ID;
extern const char SHM_PROJ_ID;
extern const int PRINT_DB;
extern const int PROCESSES_SLEEP_TIME;

// Functions
int *get_db(int additional_flags, int *shm_id_ptr);
int get_db_write_sem_id(int additional_flags);

int get_random_db_value();
void sort_db(int *db);
void print_db(int *db);
