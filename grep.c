#include "engine.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    // TODO: parse the arguments in argv.
    // You can expect argv[1] to be the mode
    // You can expect argv[2] to be the filepath
    // You can expect argv[3] to be the target word

    if (argc != 4) {
        fprintf(stderr,
                "Usage: %s <count|instance> <input_file> <target_word>\n",
                argv[0]);
        return 1;
    }

    char *mode = argv[1];
    char *readfile = argv[2];
    char *target = argv[3];

    if (strcmp(mode, "count") == 0) {
        int count_only = search_count(readfile, target);
        printf("Found: %d of \"%s\" in [%s]\n",
                count_only, target, readfile);
        return 0;
    }

    struct count_result result = search_instance(readfile, target);

    // printing to console
    printf("Found: %d of \"%s\" in [%s]\n",
            result.count, target, readfile);

    for (int i = 0; i < result.count; i++) {
        printf("Instance %d: %s\n",
                i + 1, result.instances[i]);
    }

    return 0;
}