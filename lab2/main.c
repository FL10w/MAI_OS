#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <stdbool.h>


static sem_t active_thread_cap;

static char* make_genome(size_t repeats, size_t *out_len) {
    const char motif[] = "AGCTagctGCTGT"; 
    const size_t motif_len = sizeof(motif) - 1;
    const size_t total = motif_len * repeats;

    char *buf = malloc(total + 1);
    if (!buf) return NULL;

    for (size_t i = 0; i < repeats; ++i) {
        memcpy(buf + i * motif_len, motif, motif_len);
    }
    buf[total] = '\0';
    *out_len = total;
    return buf;
}


static size_t count_matches(const char *text, const char *pattern,
                            size_t text_start, size_t text_end) {
    const size_t pat_len = strlen(pattern);
    if (pat_len == 0 || text_end <= text_start) return 0;

    size_t hits = 0;
    const size_t last_start = (text_end >= pat_len) ? (text_end - pat_len) : 0;

    for (size_t i = text_start; i <= last_start; ++i) {
        if (memcmp(&text[i], pattern, pat_len) == 0) {
            hits++;
        }
    }
    return hits;
}

typedef struct {
    const char *text;
    const char *pattern;
    size_t from;
    size_t to;
} task_t;

static void* thread_worker(void *arg) {
    task_t *job = (task_t*)arg;

    if (sem_wait(&active_thread_cap) != 0) {
        perror("sem_wait");
        return NULL;
    }

    size_t *result = malloc(sizeof(size_t));
    if (!result) {
        perror("malloc result");
        sem_post(&active_thread_cap);
        return NULL;
    }

    *result = count_matches(job->text, job->pattern, job->from, job->to);

    if (sem_post(&active_thread_cap) != 0) {
        perror("sem_post");
    }

    return result;
}

static double run_parallel(const char *text, const char *pattern,
                           size_t num_threads, size_t max_active,
                           size_t *out_total) {
    const size_t txt_len = strlen(text);
    const size_t pat_len = strlen(pattern);

    const unsigned int sem_init_val = (max_active == 0) ? num_threads : (unsigned int)max_active;
    if (sem_init(&active_thread_cap, 0, sem_init_val) != 0) {
        perror("sem_init");
        return -1.0;
    }

    pthread_t *pool = calloc(num_threads, sizeof(pthread_t));
    task_t *jobs = calloc(num_threads, sizeof(task_t));
    if (!pool || !jobs) {
        perror("calloc");
        sem_destroy(&active_thread_cap);
        free(pool);
        free(jobs);
        return -1.0;
    }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    const size_t base_chunk = txt_len / num_threads;
    for (size_t i = 0; i < num_threads; ++i) {
        jobs[i].text = text;
        jobs[i].pattern = pattern;
        jobs[i].from = i * base_chunk;

        if (i == num_threads - 1) {
            jobs[i].to = txt_len;
        } else {
            jobs[i].to = (i + 1) * base_chunk + pat_len - 1;
            if (jobs[i].to > txt_len) jobs[i].to = txt_len;
        }

        if (pthread_create(&pool[i], NULL, thread_worker, &jobs[i]) != 0) {
            perror("pthread_create");
            for (size_t j = 0; j < i; ++j) {
                void *res;
                pthread_join(pool[j], &res);
                free(res);
            }
            sem_destroy(&active_thread_cap);
            free(pool);
            free(jobs);
            return -1.0;
        }
    }

    *out_total = 0;
    for (size_t i = 0; i < num_threads; ++i) {
        void *res;
        if (pthread_join(pool[i], &res) == 0 && res) {
            *out_total += *(size_t*)res;
            free(res);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    sem_destroy(&active_thread_cap);
    free(pool);
    free(jobs);

    double ms = (t1.tv_sec - t0.tv_sec) * 1000.0;
    ms += (t1.tv_nsec - t0.tv_nsec) / 1e6;
    return ms;
}

static double run_sequential(const char *text, const char *pattern, size_t *out_count) {
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    *out_count = count_matches(text, pattern, 0, strlen(text));
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double ms = (t1.tv_sec - t0.tv_sec) * 1000.0;
    ms += (t1.tv_nsec - t0.tv_nsec) / 1e6;
    return ms;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Использование: %s <паттерн> <потоков_N> <лимит_L (0=без_лимита)>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *pattern = argv[1];
    const size_t N = strtoul(argv[2], NULL, 10);
    const size_t L = strtoul(argv[3], NULL, 10);

    if (N == 0 || strlen(pattern) == 0) {
        fprintf(stderr, "Ошибка: N > 0, паттерн не пуст.\n");
        return EXIT_FAILURE;
    }

    size_t genome_len = 0;
    char *dna = make_genome(700000, &genome_len); 
    if (!dna) {
        fprintf(stderr, "Не удалось выделить память под геном.\n");
        return EXIT_FAILURE;
    }

    size_t seq_hits = 0;
    double t_seq = run_sequential(dna, pattern, &seq_hits);
    if (t_seq < 0) {
        free(dna);
        return EXIT_FAILURE;
    }

    size_t par_hits = 0;
    double t_par = run_parallel(dna, pattern, N, L, &par_hits);
    if (t_par < 0) {
        fprintf(stderr, "Ошибка в параллельной версии.\n");
        free(dna);
        return EXIT_FAILURE;
    }

    printf("Паттерн: '%s'\n", pattern);
    printf("Последовательно: %.4f мс, найдено: %zu\n", t_seq, seq_hits);
    printf("Параллельно    : %.4f мс, найдено: %zu\n", t_par, par_hits);
    printf("Ускорение      : %.2fx\n", t_seq / t_par);

    if (seq_hits != par_hits) {
        printf("результаты не совпадают!\n");
    }

    free(dna);
    return EXIT_SUCCESS;
}