#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include "shared.h" 
#include <string.h>

static char SERVER_PROGRAM_NAME[] = "lab3";

int main(int argc, char **argv) {
    if (argc == 1) {
        char msg[1024];
        uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s filename\n", argv[0]);
        write(STDERR_FILENO, msg, len);
        exit(EXIT_SUCCESS);
    }

    char progpath[1024];
    {
        ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);
        if (len == -1) {
            const char msg[] = "error: failed to read full program path\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }
        while (progpath[len] != '/')
            --len;
        progpath[len] = '\0';
    }

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0600);
    if (shm_fd == -1) {
        const char msg[] = "error: failed to create shared memory object\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(SharedData)) == -1) {
        const char msg[] = "error: failed to set shared memory size\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    SharedData *shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        const char msg[] = "error: failed to map shared memory\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    sem_t *sem_data_ready = sem_open(SEM_DATA_READY_NAME, O_CREAT | O_EXCL, 0600, 0);
    if (sem_data_ready == SEM_FAILED) {
        const char msg[] = "error: failed to create semaphore data_ready\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        munmap(shared_data, sizeof(SharedData));
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    sem_t *sem_data_free = sem_open(SEM_DATA_FREE_NAME, O_CREAT | O_EXCL, 0600, 1); 
    if (sem_data_free == SEM_FAILED) {
        const char msg[] = "error: failed to create semaphore data_free\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        sem_close(sem_data_ready);
        sem_unlink(SEM_DATA_READY_NAME);
        munmap(shared_data, sizeof(SharedData));
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    pid_t child = fork();

    switch (child) {
        case -1: {
            const char msg[] = "error: failed to spawn new process\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            sem_close(sem_data_ready);
            sem_close(sem_data_free);
            sem_unlink(SEM_DATA_READY_NAME);
            sem_unlink(SEM_DATA_FREE_NAME);
            munmap(shared_data, sizeof(SharedData));
            close(shm_fd);
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        } break;

        case 0: { 

            close(shm_fd);

            char path[1024];
            snprintf(path, sizeof(path) - 1, "%s/%s", progpath, SERVER_PROGRAM_NAME);

            char *const args[] = {SERVER_PROGRAM_NAME, argv[1], NULL};

            int status = execv(path, args);

            if (status == -1) {
                const char msg[] = "error: failed to exec into new executable image\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }
        } break;

        default: { 
            {
                pid_t pid = getpid();
                char msg[64];
                const int32_t length = snprintf(msg, sizeof(msg),
                    "%d: I'm a parent, my child has PID %d\n", pid, child);
                write(STDOUT_FILENO, msg, length);
            }

            char buf[4096];
            ssize_t bytes;

            while (true) {
                bytes = read(STDIN_FILENO, buf, sizeof(buf) - 1);
                if (bytes < 0) {
                    const char msg[] = "error: failed to read from stdin\n";
                    write(STDERR_FILENO, msg, sizeof(msg));
                    break;
                } else if (buf[0] == '\n') {
                    break;
                }

                if (sem_wait(sem_data_free) == -1) {
                    const char msg[] = "error: failed to wait on semaphore data_free\n";
                    write(STDERR_FILENO, msg, sizeof(msg));
                    break;
                }

                if (bytes >= sizeof(shared_data->data)) {
                    bytes = sizeof(shared_data->data) - 1; 
                }
                memcpy(shared_data->data, buf, bytes);
                shared_data->data[bytes] = '\0'; 
                shared_data->ready = 1; 

                if (sem_post(sem_data_ready) == -1) {
                    const char msg[] = "error: failed to post semaphore data_ready\n";
                    write(STDERR_FILENO, msg, sizeof(msg));
                    break;
                }
            }

            wait(NULL);

            sem_close(sem_data_ready);
            sem_close(sem_data_free);
            sem_unlink(SEM_DATA_READY_NAME);
            sem_unlink(SEM_DATA_FREE_NAME);
            munmap(shared_data, sizeof(SharedData));
            close(shm_fd);
            shm_unlink(SHM_NAME);
        } break;
    }

    return 0;
}