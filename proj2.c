#include <limits.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// All semaphores
typedef enum {
    SEM_MUTEX,              // General mutex
    SEM_OXYGEN_QUEUE,       // Oxygen queue
    SEM_HYDROGEN_QUEUE,     // Hydrogen queue
    SEM_LOG_MUTEX,          // Log mutex
    SEM_MOLECULE_CREATED,   // First stage of molecule creation
    SEM_MOLECULE_CREATED2,  // Second stage of molecule creation
    SEM_MOLECULE_CREATING,
    SEM_MOLECULE_CREATING2,
    SEM_COUNT  // NOT FOR REAL USE! Just a count of all semaphores
} Sem;

// Shared memory structure
struct s_shared {
    uint32_t oxygen_count;
    uint32_t hydrogen_count;
    uint32_t molecule_count;
    uint32_t oxygen_processed;
    uint32_t hydrogen_processed;
    bool not_enough;
    uint32_t log_line_number;
    sem_t semaphores[SEM_COUNT];
    FILE* log_stream;
};

// Command line arguments structure
typedef struct arguments {
    uint32_t no;
    uint32_t nh;
    uint32_t ti;
    uint32_t tb;
} arguments_t;

// Shared memory
int shmid;
struct s_shared* shared = NULL;

// Output file stream
FILE* log_stream;

/**
 * Wait some time
 *
 * @param millis Maximum number of milliseconds to sleep
 */
void wait_rand(uint32_t millis) {
    uint32_t time = rand() % (millis + 1);
    usleep(time * 1000);
}

/**
 * Initialize all semaphores
 *
 * @return true if successful, false otherwise
 */
bool init_semaphores() {
    // Initial semaphore values
    unsigned int values[] = {
        [SEM_MUTEX] = 1,
        [SEM_OXYGEN_QUEUE] = 0,
        [SEM_HYDROGEN_QUEUE] = 0,
        [SEM_MOLECULE_CREATED] = 0,
        [SEM_MOLECULE_CREATED2] = 0,
        [SEM_MOLECULE_CREATING] = 0,
        [SEM_MOLECULE_CREATING2] = 0,
        [SEM_LOG_MUTEX] = 1,
    };

    for (int i = 0; i < SEM_COUNT; i++) {
        if (sem_init(&shared->semaphores[i], 1, values[i])) {
            return false;
        }
    }

    return true;
}

/**
 * Destroy all semaphores
 */
void destroy_semaphores() {
    for (int i = 0; i < SEM_COUNT; i++) {
        sem_destroy(&shared->semaphores[i]);
    }
}

void detach_shared() {
    shmdt(shared);
}

bool init_shared() {
    shmid = shmget(IPC_PRIVATE, sizeof(struct s_shared), IPC_CREAT | 0666);
    if (shmid == -1) {
        return false;
    }

    shared = shmat(shmid, NULL, 0);
    if (shared == (void*)-1) {
        shmctl(shmid, IPC_RMID, NULL);
        return false;
    }

    atexit(detach_shared);

    shared->log_line_number = 1;
    shared->hydrogen_count = 0;
    shared->oxygen_count = 0;
    shared->molecule_count = 0;
    shared->oxygen_processed = 0;
    shared->hydrogen_processed = 0;
    shared->not_enough = false;

    return true;
}

void destroy_shared() {
    shmctl(shmid, IPC_RMID, NULL);
}

bool open_log() {
    shared->log_stream = fopen("proj2.out", "w");
    return shared->log_stream != NULL;
}

void close_log() {
    fclose(shared->log_stream);
}

void flog(const char* fmt, ...) {
    sem_wait(&shared->semaphores[SEM_LOG_MUTEX]);

    va_list arg;
    va_start(arg, fmt);
    fprintf(shared->log_stream, "%d: ", shared->log_line_number);
    vfprintf(shared->log_stream, fmt, arg);
    va_end(arg);
    fflush(shared->log_stream);

    shared->log_line_number++;

    sem_post(&shared->semaphores[SEM_LOG_MUTEX]);
}

/**
 * @brief Parse number from string
 *
 * Exits program if number is not valid
 *
 * @param str The string to be parsed
 * @param min Minimum valid value
 * @param max Maximum valid value
 * @return Parsed number
 */
unsigned long parse_argument(char* str, size_t min, size_t max) {
    char* end;
    unsigned long number = strtoul(str, &end, 10);
    if (end == str || *end != '\0' || number > max || number < min) {
        fprintf(stderr, "Invalid argument: %s\n", str);
        exit(EXIT_FAILURE);
    }
    return number;
}

void oxygen_process(uint32_t id, arguments_t args) {
    srand(time(NULL) + id * 100 + getpid());
    flog("O %d: started\n", id);
    wait_rand(args.ti);
    flog("O %d: going to queue\n", id);

    sem_wait(&shared->semaphores[SEM_MUTEX]);

    shared->oxygen_count++;

    if (shared->hydrogen_count >= 2) {
        sem_post(&shared->semaphores[SEM_HYDROGEN_QUEUE]);
        sem_post(&shared->semaphores[SEM_HYDROGEN_QUEUE]);
        shared->hydrogen_count -= 2;
        sem_post(&shared->semaphores[SEM_OXYGEN_QUEUE]);
        shared->oxygen_count--;
    } else {
        sem_post(&shared->semaphores[SEM_MUTEX]);
    }

    sem_wait(&shared->semaphores[SEM_OXYGEN_QUEUE]);

    if (shared->not_enough) {
        flog("O %d: not enough H\n", id);
        sem_post(&shared->semaphores[SEM_MUTEX]);
        sem_post(&shared->semaphores[SEM_OXYGEN_QUEUE]);
        sem_post(&shared->semaphores[SEM_HYDROGEN_QUEUE]);
        exit(0);
    }

    // Init molecule creation
    flog("O %d: creating molecule %d\n", id, shared->molecule_count + 1);

    // Create molecule (by waiting)
    wait_rand(args.tb);
    shared->molecule_count++;

    sem_wait(&shared->semaphores[SEM_MOLECULE_CREATING]);

    // Print created
    shared->oxygen_processed++;

    // Signalize creation to hydrogen processes
    sem_post(&shared->semaphores[SEM_MOLECULE_CREATED]);
    sem_post(&shared->semaphores[SEM_MOLECULE_CREATED]);

    sem_wait(&shared->semaphores[SEM_MOLECULE_CREATED2]);
    sem_wait(&shared->semaphores[SEM_MOLECULE_CREATED2]);

    flog("O %d: molecule %d created\n", id, shared->molecule_count);

    if (args.no - shared->oxygen_processed >= 1 && args.nh - shared->hydrogen_processed < 2) {
        shared->not_enough = true;
        sem_post(&shared->semaphores[SEM_OXYGEN_QUEUE]);
    } else if (args.no - shared->oxygen_processed == 0 &&
               args.nh - shared->hydrogen_processed > 0) {
        shared->not_enough = true;
        sem_post(&shared->semaphores[SEM_HYDROGEN_QUEUE]);
    }

    // Unlock mutex
    sem_post(&shared->semaphores[SEM_MUTEX]);

    exit(0);
}

void hydrogen_process(uint32_t id, arguments_t args) {
    srand(time(NULL) + id * 100 + getpid());

    flog("H %d: started\n", id);
    wait_rand(args.ti);
    flog("H %d: going to queue\n", id);

    sem_wait(&shared->semaphores[SEM_MUTEX]);

    // printf("%d H: %d\n", id, shared->hydrogen_processed);

    shared->hydrogen_count++;

    if (shared->hydrogen_count >= 2 && shared->oxygen_count >= 1) {
        sem_post(&shared->semaphores[SEM_HYDROGEN_QUEUE]);
        sem_post(&shared->semaphores[SEM_HYDROGEN_QUEUE]);
        shared->hydrogen_count -= 2;
        sem_post(&shared->semaphores[SEM_OXYGEN_QUEUE]);
        shared->oxygen_count--;
    } else {
        sem_post(&shared->semaphores[SEM_MUTEX]);
    }

    sem_wait(&shared->semaphores[SEM_HYDROGEN_QUEUE]);

    if (shared->not_enough) {
        flog("H %d: not enough O or H\n", id);
        sem_post(&shared->semaphores[SEM_OXYGEN_QUEUE]);
        sem_post(&shared->semaphores[SEM_HYDROGEN_QUEUE]);
        exit(0);
    }

    // Init molecule creation
    flog("H %d: creating molecule %d\n", id, shared->molecule_count + 1);

    sem_post(&shared->semaphores[SEM_MOLECULE_CREATING]);

    // Wait for oxygen process to create the molecule
    sem_wait(&shared->semaphores[SEM_MOLECULE_CREATED]);

    // Molecule has been created
    flog("H %d: molecule %d created\n", id, shared->molecule_count);
    shared->hydrogen_processed++;

    sem_post(&shared->semaphores[SEM_MOLECULE_CREATED2]);

    exit(0);
}

int main(int argc, char* argv[]) {
    // Check number of arguments
    if (argc != 5) {
        fprintf(stderr, "Invalid number of arguments!\n");
        return 1;
    }

    // Initialize
    if (!init_shared()) {
        fprintf(stderr, "Could not initialize shared memory\n");
        goto shared_error;
    }
    if (!open_log()) {
        fprintf(stderr, "Could not open log file\n");
        goto log_error;
    }
    if (!init_semaphores()) {
        fprintf(stderr, "Could not initialize semaphores\n");
        goto sem_error;
    }

    unsigned long spawned = 0;

    // Parse arguments
    arguments_t args = {.no = parse_argument(argv[1], 1, LONG_MAX),
                        .nh = parse_argument(argv[2], 1, LONG_MAX),
                        .ti = parse_argument(argv[3], 0, 1000),
                        .tb = parse_argument(argv[4], 0, 1000)};

    // Children pid array
    pid_t* pids = malloc(sizeof(pid_t) * (args.no + args.nh));
    if (pids == NULL) {
        fprintf(stderr, "Malloc error\n");
        return 1;
    }

    // Check if at least one molecule can be created
    if (args.no == 0 || args.nh < 2) {
        shared->not_enough = true;
        sem_post(&shared->semaphores[SEM_OXYGEN_QUEUE]);
        sem_post(&shared->semaphores[SEM_HYDROGEN_QUEUE]);
    }

    // Spawn oxygen processes
    for (unsigned long i = 0; i < args.no; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            oxygen_process(i + 1, args);
            return 0;
        } else if (pid == -1) {
            goto fork_error;
        } else {
            pids[spawned] = pid;
        }
        spawned++;
    }

    // Spawn hydrogen processes
    for (unsigned long i = 0; i < args.nh; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            hydrogen_process(i + 1, args);
            return 0;
        } else if (pid == -1) {
            goto fork_error;
        } else {
            pids[spawned] = pid;
        }
        spawned++;
    }

    // Wait for all children to end
    for (uint32_t i = 0; i < spawned; i++) {
        wait(NULL);
    }

    // Cleanup
    close_log();
    destroy_semaphores();
    destroy_shared();

    return 0;

// Error handling section
fork_error:
    fprintf(stderr, "Fork error\n");

    // Kill all children
    for (uint32_t i = 0; i < spawned; i++) {
        kill(pids[i], SIGKILL);
    }
    free(pids);

    // Cleanup
sem_error:
    destroy_semaphores();
log_error:
    close_log();
shared_error:
    destroy_shared();

    return 1;
}
