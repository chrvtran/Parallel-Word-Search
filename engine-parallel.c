#include "engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256
#define NUM_THREADS 8
// threads 2 4 8 may work differently depending on size

struct worker_args {
    char *filename;
    char *target;

    long start; //start of the chunk to process
    long end; //end of the chunk to process
};

char *trim(char *line) {
    char *ptr = line;
    // moves pointer to non-white space
    while (isspace((unsigned char)*ptr)) {
        ptr++;
    }
    // finds ending non-white space
    char *end = ptr + strlen(ptr);
    while (end > ptr && isspace((unsigned char)*(end - 1))) {
        end--;
    }
    // moves original pointer
    int length = end - ptr;
    for (int i = 0; i < length; i++) {
        line[i] = ptr[i];
    }
    line[length] = '\0';
    return line;
}

// see appx.md
void *instance_worker(void *arg) {
    struct worker_args *args = arg;
    struct count_result *result = calloc(1, sizeof(*result));
    if (result == NULL) {
        return NULL;
    }

    FILE *file = fopen(args->filename, "rb");
    if (file == NULL) {
        free(result);
        return NULL;
    }

    // seek/skip if start is not beginning cuz partial line
    if (args->start > 0) {
        if (fseek(file, args->start - 1, SEEK_SET) != 0) {
            // file; shift; policy
            fclose(file);
            free(result);
            return NULL;
        }
        int previous_char = fgetc(file);
        if (previous_char != '\n') {
            int skipped_char;
            while ((skipped_char = fgetc(file)) != '\n' && skipped_char != EOF) {
                // reach end of line by skipping
            }
        }
    }

    char *line = NULL;
    size_t length = 0;

    while (getline(&line, &length, file) != -1) {
        // check length
        long line_start = ftell(file) - (long)strlen(line);
        if (line_start >= args->end) break;

        char *word = line;
        while ((word = strstr(word, args->target)) != NULL) {
            // allocate, trim, store
            result->instances = realloc(result->instances, (size_t)(result->count + 1) * sizeof(char *));
            char *copy = strdup(line);
            result->instances[result->count] = trim(copy);

            // next
            result->count++;
            word += strlen(args->target);
        }
    }

    free(line);
    fclose(file);
    return result;
}

void *count_worker(void *arg) {
    struct worker_args *args = arg;
    (void)args;
    // i don't need this function

    return NULL;
}

// OG BELOW
int search_count(char *filename, char *target) {
    struct count_result result = search_instance(filename, target);
    int count = result.count;

    for (int i = 0; i < result.count; i++) {
        free(result.instances[i]);
    }
    free(result.instances);

    return count;
}

// see appx.md
struct count_result search_instance(char *filename,char *target){
    struct count_result result = {0, NULL};

    // open file for byte read
    FILE *file = fopen(filename, "rb");
    if (file == NULL || strcmp(target, "") == 0) {
        return result;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return result;
    }

    // determining file size
    long file_size = ftell(file);
    fclose(file);
    if (file_size < 0) return result;

    pthread_t threads[NUM_THREADS];
    struct worker_args args[NUM_THREADS];
    struct count_result *worker_results[NUM_THREADS] = {NULL}; // array of workers

    // divide bytes into threads
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].filename = filename;
        args[i].target = target;
        args[i].start = file_size * i / NUM_THREADS;
        args[i].end = file_size * (i + 1) / NUM_THREADS;
        pthread_create(
            &threads[i], // which thread
            NULL, // default settings
            instance_worker, // worker function
            &args[i] // pointer to argument structure
        );
    }
    
    // wait for thread completion
    for (int i = 0; i < NUM_THREADS; i++) {
        void *thread_result = NULL;
        pthread_join(threads[i], &thread_result);
        worker_results[i] = thread_result;
        result.count += worker_results[i]->count;
    }

    // allocate space for all instances
    if (result.count > 0) {
        result.instances = malloc((size_t)result.count * sizeof(char *));
        // if a thread failed
        if (result.instances == NULL) {
            // free cuz fail
            for (int i = 0; i < NUM_THREADS; i++) {
                if (worker_results[i] != NULL) {
                    for (int j = 0; j < worker_results[i]->count; j++) {
                        free(worker_results[i]->instances[j]);
                    }
                    free(worker_results[i]->instances);
                    free(worker_results[i]);
                }
            }
            free(result.instances);
            result.count = 0;
            result.instances = NULL;

            return result;
        }
    }

    // otherwise, combine results!
    int index = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        struct count_result *worker = worker_results[i];
        for (int j = 0; j < worker->count; j++) {
            result.instances[index++] = worker->instances[j];
        }

        free(worker->instances);
        free(worker);
        worker_results[i] = NULL;
    }

    return result;
}