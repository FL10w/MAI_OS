#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "shared.h" 
#include <string.h>

float bober(char buf[]) {
    float numbers[100];
    int count = 0;
    int i = 0;

    while (buf[i] != '\0' && buf[i] != '\n') {
        while (buf[i] == ' ') i++;

        if (buf[i] == '\0' || buf[i] == '\n') break;

        float num = 0;
        int is_negative = 0;
        int has_decimal = 0;
        float decimal_place = 0.1f;

        if (buf[i] == '-') {
            is_negative = 1;
            i++;
        }

        while ((buf[i] >= '0' && buf[i] <= '9') || buf[i] == '.') {
            if (buf[i] == '.') {
                has_decimal = 1;
            } else {
                if (!has_decimal) {
                    num = num * 10 + (buf[i] - '0');
                } else {
                    num += (buf[i] - '0') * decimal_place;
                    decimal_place *= 0.1f;
                }
            }
            i++;
        }

        if (is_negative) {
            num = -num;
        }

        numbers[count++] = num;
        while (buf[i] == ' ') i++;
    }

    if (count == 0) {
        return 0;
    }

    if (count == 1) {
        return numbers[0];
    }

    float result = numbers[0];
    for (int j = 1; j < count; j++) {
        if (numbers[j] == 0) {
            const char msg[] = "error: division by zero\n";
            write(STDOUT_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE); 
        }
        result /= numbers[j];
    }

    return result;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        const char msg[] = "error: filename argument required\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (shm_fd == -1) {
        const char msg[] = "error: failed to open shared memory object\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    SharedData *shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        const char msg[] = "error: failed to map shared memory\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    sem_t *sem_data_ready = sem_open(SEM_DATA_READY_NAME, 0);
    if (sem_data_ready == SEM_FAILED) {
        const char msg[] = "error: failed to open semaphore data_ready\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        munmap(shared_data, sizeof(SharedData));
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    sem_t *sem_data_free = sem_open(SEM_DATA_FREE_NAME, 0);
    if (sem_data_free == SEM_FAILED) {
        const char msg[] = "error: failed to open semaphore data_free\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        sem_close(sem_data_ready);
        munmap(shared_data, sizeof(SharedData));
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    int file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (file == -1) {
        const char msg[] = "error: failed to open requested file\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
        sem_close(sem_data_ready);
        sem_close(sem_data_free);
        munmap(shared_data, sizeof(SharedData));
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    while (true) {
        if (sem_wait(sem_data_ready) == -1) {
            const char msg[] = "error: failed to wait on semaphore data_ready\n";
            write(STDOUT_FILENO, msg, sizeof(msg));
            break;
        }

        if (shared_data->ready) {
            char local_buf[4096];
            strncpy(local_buf, shared_data->data, sizeof(local_buf) - 1);
            local_buf[sizeof(local_buf) - 1] = '\0';

            shared_data->ready = 0;

            if (sem_post(sem_data_free) == -1) {
                const char msg[] = "error: failed to post semaphore data_free\n";
                write(STDOUT_FILENO, msg, sizeof(msg));
                break;
            }

            float result = bober(local_buf);
            char result_str[128];
            int bytes = snprintf(result_str, sizeof(result_str), "%.4f\n", result);

            if (write(file, result_str, bytes) != bytes) {
                const char msg[] = "error: failed to write to file\n";
                write(STDOUT_FILENO, msg, sizeof(msg));
                break;
            }

            const char success_msg[] = "success\n";
            write(STDOUT_FILENO, success_msg, sizeof(success_msg));
        }
    }

    sem_close(sem_data_ready);
    sem_close(sem_data_free);
    munmap(shared_data, sizeof(SharedData));
    close(shm_fd);
    close(file);

    return 0;
}