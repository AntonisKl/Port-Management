#include "vessel.h"

void handleFlags(int argc, char** argv, char* type, char* upgradeFlag, suseconds_t* parkTime, suseconds_t* manTime, int* shmId, char** logFileName) {
    if (argc != 13) {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    char* ptr;

    if (strcmp(argv[1], "-t") == 0) {
        (*type) = argv[2][0];
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    if (strcmp(argv[3], "-u") == 0) {
        int upgradeFlagInt = atoi(argv[4]);
        if (upgradeFlagInt == 0)
            (*upgradeFlag) = 0;
        else if (upgradeFlagInt == 1)
            (*upgradeFlag) = 1;
        else {
            printf("Invalid upgrade flag\nExiting...\n");
            exit(1);
        }

    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    if (strcmp(argv[5], "-p") == 0) {
        long parkTimeLong = strtol(argv[6], &ptr, 10);
        if (parkTimeLong == 0) {
            printf("Invalid park time argument\nExiting...\n");
            exit(1);
        }

        (*parkTime) = parkTimeLong;
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    if (strcmp(argv[7], "-m") == 0) {
        long manTimeLong = strtol(argv[8], &ptr, 10);
        if (manTimeLong == 0) {
            printf("Invalid park time argument\nExiting...\n");
            exit(1);
        }

        (*manTime) = manTimeLong;
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    if (strcmp(argv[9], "-s") == 0) {
        int shmIdInt = atoi(argv[10]);
        if (shmIdInt == 0) {
            printf("Invalid shared memory id argument\nExiting...\n");
            exit(1);
        }

        (*shmId) = shmIdInt;
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    if (strcmp(argv[11], "-lf") == 0) {
        (*logFileName) = argv[12];
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }
}

sem_t* getShipTypeSem(SharedMemory* sharedMemory, char shipType) {
    for (unsigned int i = 0; i < 3; i++) {
        if (shipType == sharedMemory->parkingSpotGroups[i].type) {
            return &sharedMemory->shipTypesSem[i];
        }
    }
    printf("Oops\n");
    return SEM_FAILED;
}

void addShipNodeToShm(SharedMemory* sharedMemory, char shipType, char upgradeFlag, suseconds_t parkTime, suseconds_t manTime) {
    unsigned int nextShipNodeIndex = sharedMemory->nextShipNodeIndex;

    //  NEED SEMAPHORE FOR WRITING TO SHARED MEMORY <--------------------------------------------------------------------------

    sharedMemory->shipNodes[nextShipNodeIndex].shipType = shipType;
    sharedMemory->shipNodes[nextShipNodeIndex].parkTimePeriod = parkTime;
    // sharedMemory->shipNodes[nextShipNodeIndex].arrivalTime = //now ?????????????? or when it parks--- maybe when it parks
    sharedMemory->shipNodes[nextShipNodeIndex].shipId = getpid();
    sharedMemory->shipNodes[nextShipNodeIndex].manTimePeriod = manTime;
    sharedMemory->shipNodes[nextShipNodeIndex].upgradeFlag = upgradeFlag;

    sharedMemory->nextShipNodeIndex++;
    // sharedMemory->shipNodes[nextShipNodeIndex].stayCost
}

int main(int argc, char** argv) {
    char shipType, upgradeFlag, *logFileName;
    int shmId;
    suseconds_t parkTimePeriod, manTimePeriod;

    handleFlags(argc, argv, &shipType, &upgradeFlag, &parkTimePeriod, &manTimePeriod, &shmId, &logFileName);

    SharedMemory* sharedMemory = (SharedMemory*)shmat(shmId, NULL, 0);
    doShifts(sharedMemory, sharedMemory->sizeOfShipNodes);  // do necessary shifts

    sem_t shipTypeSem = *getShipTypeSem(sharedMemory, shipType);

    // add a ship node to shared memory
    addShipNodeToShm(sharedMemory, shipType, upgradeFlag, parkTimePeriod, manTimePeriod);

    sem_wait(&shipTypeSem);

    //sleep(); // sleep for manTimePeriod + parkTimePeriod


    // maybe destroy local semaphore ???????

    return 0;
}