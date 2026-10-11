#include "engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256

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

int search_count(char *filename, char *target) {
    // kinda redundant...
    struct count_result result = search_instance(filename, target);
    int count = result.count;

    for (int i = 0; i < result.count; i++) {
        free(result.instances[i]);
    }
    free(result.instances);

    return count;
}

struct count_result search_instance(char *filename, char *target){
    struct count_result result = {0, NULL};

    FILE *file = fopen(filename, "r");
    if (file == NULL || strcmp(target, "") == 0) {
        return result;
    }

    char *line = NULL;
    size_t length = 0;

    while (getline(&line, &length, file) != -1) {        
        char *word = line;
        // checks each word occurence
        while ((word = strstr(word, target)) != NULL) {
            // allocate space for new entry
            result.instances = realloc(result.instances, (result.count + 1) * sizeof(char *));

            // store entire line (repeats for multiple occurences)
            char *copy = strdup(line);
            result.instances[result.count] = trim(copy);
            
            // next
            result.count++;
            word += strlen(target);
        }
    }

    free(line);
    fclose(file);
    return result;
}