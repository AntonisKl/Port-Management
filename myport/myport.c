#include "myport.h"

void handleFlags(int argc, char** argv, char** configFileName) {
    if (argc != 3) {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    char* ptr;

    if (strcmp(argv[1], "-l") == 0) {
        (*configFileName) = argv[2];
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }
}