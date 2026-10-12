#include "engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <omp.h>
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
static struct count_result search_instance_chunk(struct worker_args *arg) {
    struct worker_args *args = arg;
    struct count_result result = {0, NULL};

    FILE *file = fopen(args->filename, "rb");
    if (file == NULL) return result;

    // seek/skip if start is not beginning cuz partial line
    if (args->start > 0) {
        if (fseek(file, args->start - 1, SEEK_SET) != 0) {
            fclose(file);
            return result;
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
            result.instances = realloc(result.instances, (size_t)(result.count + 1) * sizeof(char *));
            char *copy = strdup(line);
            result.instances[result.count] = trim(copy);

            // next
            result.count++;
            word += strlen(args->target);
        }
    }

    free(line);
    fclose(file);
    return result;
}

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

    struct worker_args args[NUM_THREADS];
    struct count_result partial[NUM_THREADS] = {0};

    // divide bytes into threads
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].filename = filename;
        args[i].target = target;
        args[i].start = file_size * i / NUM_THREADS;
        args[i].end = file_size * (i + 1) / NUM_THREADS;
    }
    
    // OpenMP (Open Multi-Processing)
    #pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++) {
        partial[i] = search_instance_chunk(&args[i]); // TODO: calculate start/end based on i and size of the file
    }

    // allocate space for all instances
    for (int i = 0; i < NUM_THREADS; i++) {
        for (int j = 0; j < partial[i].count; j++) {
            result.instances = realloc(result.instances, (size_t)(result.count + 1) * sizeof(char *));

            // combine results!
            result.instances[result.count] = partial[i].instances[j];
            result.count++;
        }

        free(partial[i].instances);
    }

    return result;
}