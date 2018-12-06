#include "monitor.h"

void handleFlags(int argc, char** argv, suseconds_t* statusPrintTime, suseconds_t* statsPrintTime, int* shmId) {
    if (argc != 7) {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    char* ptr;

    if (strcmp(argv[1], "-d") == 0) {
        long statusPrintTimeLong = strtol(argv[2], &ptr, 10);
        if (statusPrintTimeLong == 0) {
            printf("Invalid first time argument\nExiting...\n");
            exit(1);
        }

        (*statusPrintTime) = statusPrintTimeLong;
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    if (strcmp(argv[3], "-t") == 0) {
        long statsPrintTimeLong = strtol(argv[4], &ptr, 10);
        if (statsPrintTimeLong == 0) {
            printf("Invalid second time argument\nExiting...\n");
            exit(1);
        }

        (*statsPrintTime) = statsPrintTimeLong;
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    if (strcmp(argv[5], "-s") == 0) {
        int shmIdInt = atoi(argv[6]);
        if (shmIdInt == 0) {
            printf("Invalid shared memory id argument\nExiting...\n");
            exit(1);
        }

        (*shmId) = shmIdInt;
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }
}
