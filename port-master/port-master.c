#include "port-master.h"

void handleFlags(int argc, char** argv, /* char** chargesFileName,*/ int* shmId, char** logFileName) {
    if (argc != 5) {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    char* ptr;

    // if (strcmp(argv[1], "-c") == 0) {
    //     (*chargesFileName) = argv[2];
    // } else {
    //     printf("Invalid flags\nExiting...\n");
    //     exit(1);
    // }

    if (strcmp(argv[1], "-s") == 0) {
        int shmIdInt = atoi(argv[2]);
        if (shmIdInt == 0) {
            printf("Invalid shared memory id argument\nExiting...\n");
            exit(1);
        }
        (*shmId) = shmIdInt;
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    if (strcmp(argv[3], "-lf") == 0) {
        (*logFileName) = argv[4];
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }
}

int main(int argc, char** argv) {
    int shmId;
    char* logFileName;
    handleFlags(argc, argv, &shmId, &logFileName);


}