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

        (*parkTime) = parkTimeLong * 1000;  // convert to microseconds
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

        (*manTime) = manTimeLong * 1000;  // convert to microseconds
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

void getShipTypeSems(SharedMemory* sharedMemory, char shipType, sem_t* shipTypeSemIn /*, sem_t* shipTypeSemOut*/) {
    for (unsigned int i = 0; i < 3; i++) {
        if (shipType == sharedMemory->parkingSpotGroups[i].type) {
            shipTypeSemIn = &sharedMemory->shipTypesSemIn[i];
            // shipTypeSemOut = &sharedMemory->shipTypesSemOut[i];
            break;
        }
    }
    printf("Oops\n");
    return;
}

ShipNode* addShipNodeToShmByShipNode(SharedMemory* sharedMemory, ShipNode* shipNode) {
    unsigned int nextShipNodeIndex = sharedMemory->sizeIn;

    sem_wait(&sharedMemory->shmWriteSem);

    ShipNode* newShipNode = &sharedMemory->shipNodes[nextShipNodeIndex];

    newShipNode->shipType = shipNode->shipType;
    newShipNode->parkTimePeriod = shipNode->parkTimePeriod;
    // sharedMemory->shipNodes[nextShipNodeIndex].arrivalTime = //now ?????????????? or when it parks--- maybe when it parks
    newShipNode->shipId = getpid();
    newShipNode->manTimePeriod = shipNode->manTimePeriod;
    newShipNode->upgradeFlag = shipNode->upgradeFlag;
    newShipNode->state = shipNode->state;
    if (shipNode->ledgerShipNode != NULL)
        newShipNode->ledgerShipNode = &shipNode->ledgerShipNode;  // without "&" ???????????????????????????????????????
    else
        newShipNode->ledgerShipNode = NULL;

    sharedMemory->sizeIn++;

    sem_post(&sharedMemory->shmWriteSem);

    return newShipNode;
    // sharedMemory->shipNodes[nextShipNodeIndex].stayCost
}

ShipNode* addShipNodeToShmByValues(SharedMemory* sharedMemory, char shipType, char upgradeFlag, suseconds_t parkTimePeriod, suseconds_t manTimePeriod, State state) {
    unsigned int nextShipNodeIndex = sharedMemory->sizeIn;

    sem_wait(&sharedMemory->shmWriteSem);

    ShipNode* newShipNode = &sharedMemory->shipNodes[nextShipNodeIndex];

    newShipNode->shipType = shipType;
    newShipNode->parkTimePeriod = parkTimePeriod;
    // sharedMemory->shipNodes[nextShipNodeIndex].arrivalTime = //now ?????????????? or when it parks--- maybe when it parks
    newShipNode->shipId = getpid();
    newShipNode->manTimePeriod = manTimePeriod;
    newShipNode->upgradeFlag = upgradeFlag;
    newShipNode->state = state;
    // if (shipNode->ledgerShipNode != NULL)
    //     newShipNode->ledgerShipNode = &shipNode->ledgerShipNode;  // without "&" ???????????????????????????????????????
    // else

    // first time adding vessel to queue so ledgerShipNode is NULL
    newShipNode->ledgerShipNode = NULL;

    sharedMemory->sizeIn++;

    sem_post(&sharedMemory->shmWriteSem);

    return newShipNode;
    // sharedMemory->shipNodes[nextShipNodeIndex].stayCost
}

// void addOutShipNodeToShm(SharedMemory* sharedMemory, ShipNode* shipNode) {
//     sharedMemory->shipNodesOut[sharedMemory->nextOutShipNodeIndex] = shipNode;
//     sharedMemory->nextOutShipNodeIndex++;

//     return;
// }

// void addShipNodeToShm(SharedMemory* sharedMemory, ShipNode* shipNode, State state) {
//     unsigned int nextShipNodeIndex = sharedMemory->sizeIn;

//     sem_wait(&sharedMemory->shmWriteSem);

//     ShipNode* shipNode = &sharedMemory->shipNodes[nextShipNodeIndex];

//     shipNode->shipType = shipNode->shipType;
//     shipNode->parkTimePeriod = shipNode;
//     // sharedMemory->shipNodes[nextShipNodeIndex].arrivalTime = //now ?????????????? or when it parks--- maybe when it parks
//     shipNode->shipId = getpid();
//     shipNode->manTimePeriod = manTime;
//     shipNode->upgradeFlag = upgradeFlag;
//     shipNode->state = state;
//     sharedMemory->sizeIn++;

//     sem_post(&sharedMemory->shmWriteSem);
// }

int main(int argc, char** argv) {
    char shipType, upgradeFlag, *logFileName;
    int shmId;
    suseconds_t parkTimePeriodUsecs, manTimePeriodUsecs;

    handleFlags(argc, argv, &shipType, &upgradeFlag, &parkTimePeriodUsecs, &manTimePeriodUsecs, &shmId, &logFileName);

    // open log file .....................................................................................

    SharedMemory* sharedMemory = (SharedMemory*)shmat(shmId, NULL, 0);
    doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts

    sem_t* shipTypeSemIn = SEM_FAILED /*, *shipTypeSemOut = SEM_FAILED*/;
    getShipTypeSems(sharedMemory, shipType, shipTypeSemIn /*, shipTypeSemOut*/);
    sem_t* inOutSem = &sharedMemory->inOutSem;

    // add a ship node to shared memory
    ShipNode* shipNode = addShipNodeToShmByValues(sharedMemory, shipType, upgradeFlag, parkTimePeriodUsecs, manTimePeriodUsecs, WaitingToEnter);

    // shipNode->state = WaitingToEnter;
    sem_wait(shipTypeSemIn);

    usleep(manTimePeriodUsecs);  // sleep for manTimePeriod + parkTimePeriod

    sem_post(inOutSem);
    shipNode->state = Parked;

    usleep(parkTimePeriodUsecs);  // here maybe ask the port-master for the cost ????????????????????????????????

    // shipNode->state = WaitingToLeave;
    // go to queue again
    // same shipNode pointer as above
    shipNode->state = WaitingToLeave;
    shipNode = addShipNodeToShmByShipNode(sharedMemory, shipNode);
    sem_wait(shipTypeSemIn);

    usleep(manTimePeriodUsecs);
    sem_post(inOutSem);

    shipNode->state = Completed;

    // maybe destroy local semaphores ???????

    return 0;
}