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
        (*upgradeFlag) = argv[4][0];
        // int upgradeFlagInt = atoi(argv[4]);
        // if (upgradeFlagInt == 0)
        //     (*upgradeFlag) = 0;
        // else if (upgradeFlagInt == 1)
        //     (*upgradeFlag) = 1;
        // else {
        //     printf("Invalid upgrade flag\nExiting...\n");
        //     exit(1);
        // }
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

// void getShipTypeSem(SharedMemory* sharedMemory, char shipType, sem_t* shipTypeSemIn /*, sem_t* shipTypeSemOut*/) {
//     for (unsigned int i = 0; i < 3; i++) {
//         if (shipType == sharedMemory->parkingSpotGroups[i].type) {
//             shipTypeSemIn = &sharedMemory->shipTypesSemIn[i];
//             // shipTypeSemOut = &sharedMemory->shipTypesSemOut[i];
//             break;
//         }
//     }
//     printf("Oops\n");
//     return;
// }

ShipNode* addShipNodeToShmByShipNode(SharedMemory* sharedMemory, ShipNode* shipNodes, ShipNode shipNode) {
    sem_wait(&sharedMemory->shmWriteSem);
    unsigned int nextShipNodeIndex = sharedMemory->sizeIn;

    // ShipNode* newShipNode = &sharedMemory->shipNodes[nextShipNodeIndex];

    shipNodes[nextShipNodeIndex].shipType = shipNode.shipType;
    shipNodes[nextShipNodeIndex].parkTimePeriod = shipNode.parkTimePeriod;
    // shipNodes[nextShipNodeIndex].arrivalTime = //now ?????????????? or when it parks--- maybe when it parks
    shipNodes[nextShipNodeIndex].shipId = getpid();
    shipNodes[nextShipNodeIndex].manTimePeriod = shipNode.manTimePeriod;
    shipNodes[nextShipNodeIndex].upgradeFlag = shipNode.upgradeFlag;
    shipNodes[nextShipNodeIndex].state = shipNode.state;
    if (shipNode.ledgerShipNode != NULL)
        shipNodes[nextShipNodeIndex].ledgerShipNode = shipNode.ledgerShipNode;  // without "&" ???????????????????????????????????????
    else
        shipNodes[nextShipNodeIndex].ledgerShipNode = NULL;

    sharedMemory->sizeIn++;

    sem_post(&sharedMemory->shmWriteSem);

    return &shipNodes[nextShipNodeIndex];
    // sharedMemory->shipNodes[nextShipNodeIndex].stayCost
}

ShipNode* addShipNodeToShmByValues(SharedMemory* sharedMemory, ShipNode* shipNodes, char shipType, char upgradeFlag, suseconds_t parkTimePeriod, suseconds_t manTimePeriod, State state) {
    printf("vessel waiting to write to shm\n");
    // doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts

    sem_wait(&sharedMemory->shmWriteSem);
    unsigned int nextShipNodeIndex = sharedMemory->sizeIn;
    // doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts
    // printf("adding vessel to shm (after wait) index = %u\n", nextShipNodeIndex);
    // printf("nextshipnodeindex = %u\n", nextShipNodeIndex);
    // ShipNode* newShipNode = &sharedMemory->shipNodes[nextShipNodeIndex];
    // printf("1\n");
    // printf("56\n");
    // doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);
    shipNodes[nextShipNodeIndex].shipType = shipType;
    // printf("5\n");
    shipNodes[nextShipNodeIndex].parkTimePeriod = parkTimePeriod;
    // sharedMemory->shipNodes[nextShipNodeIndex].arrivalTime = //now ?????????????? or when it parks--- maybe when it parks
    // printf("4\n");
    shipNodes[nextShipNodeIndex].shipId = getpid();
    // printf("3\n");
    shipNodes[nextShipNodeIndex].manTimePeriod = manTimePeriod;
    shipNodes[nextShipNodeIndex].upgradeFlag = upgradeFlag;
    shipNodes[nextShipNodeIndex].state = state;
    // if (shipNode->ledgerShipNode != NULL)
    //     newShipNode->ledgerShipNode = &shipNode->ledgerShipNode;  // without "&" ???????????????????????????????????????
    // else
    // printf("2\n");

    // first time adding vessel to queue so ledgerShipNode is NULL
    shipNodes[nextShipNodeIndex].ledgerShipNode = NULL;

    sharedMemory->sizeIn++;
    // printf("adding vessel to shm (before post)\n");

    if (sem_post(&sharedMemory->shmWriteSem) < 0)
        perror("sem post failed");
    // printf("adding vessel to shm (after post) | mantime = %lu\n", shipNodes[nextShipNodeIndex].manTimePeriod);
    printf("vessel added to shared memory with type: %c --------------------------------------------------------------\n", shipNodes[nextShipNodeIndex].shipType);
    return &shipNodes[nextShipNodeIndex];
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
    printf("Vessel with pid: %d started execution\n", getpid());

    char shipType, upgradeFlag, *logFileName;
    int shmId;
    suseconds_t parkTimePeriodUsecs, manTimePeriodUsecs;

    handleFlags(argc, argv, &shipType, &upgradeFlag, &parkTimePeriodUsecs, &manTimePeriodUsecs, &shmId, &logFileName);

    // open log file .....................................................................................

    SharedMemory* sharedMemory = (SharedMemory*)((uint8_t*)shmat(shmId, 0, 0));  // error checking add <-------------------

    PublicLedger* publicLedger = (PublicLedger*)((uint8_t*)shmat(sharedMemory->shmIdPublicLedger, 0, 0));
    ParkingSpotGroup* parkingSpotGroups = (ParkingSpotGroup*)((uint8_t*)shmat(sharedMemory->shmIdParkingSpotGroups, 0, 0));
    ShipNode* shipNodes = (ShipNode*)((uint8_t*)shmat(sharedMemory->shmIdShipNodes, 0, 0));
    LedgerShipNode* ledgerShipNodes = (LedgerShipNode*)((uint8_t*)shmat(sharedMemory->shmIdLedgerNodes, 0, 0));

    // sharedMemory->publicLedger = (PublicLedger*)((uint8_t*)shmat(sharedMemory->shmIdPublicLedger, 0, 0));
    // sharedMemory->parkingSpotGroups = (ParkingSpotGroup*)((uint8_t*)shmat(sharedMemory->shmIdParkingSpotGroups, 0, 0));
    // sharedMemory->shipNodes = (ShipNode*)((uint8_t*)shmat(sharedMemory->shmIdShipNodes, 0, 0));
    // sharedMemory->publicLedger->ledgerShipNodes = (LedgerShipNode*)((uint8_t*)shmat(sharedMemory->shmIdLedgerNodes, 0, 0));

    // printf("shm address: %p\n", (void*)&sharedMemory);

    // doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts

    sem_t *shipTypeSemIn = SEM_FAILED, *shipTypeSemPending = SEM_FAILED;
    // getShipTypeSems(sharedMemory, shipTypeSemIn, shipTypeSemPending, shipType);
    sem_t* inOutSem = &sharedMemory->inOutSem;
    // printf("posted1\n");
    unsigned int shipIndex = sharedMemory->sizeIn;
    FILE* logFileP = fopen(logFileName, "a");

    // add a ship node to shared memory
    ShipNode* shipNode = addShipNodeToShmByValues(sharedMemory, shipNodes, shipType, upgradeFlag, parkTimePeriodUsecs, manTimePeriodUsecs, WaitingToEnter);
    // printf("SIZE=%u\n", sharedMemory->sizeIn);

    // shipNode->state = WaitingToEnter;
    if (sem_post(&sharedMemory->portMasterWakeSem) < 0)
        perror("portMasterWakeSem failed");
    printf("vessel with pid: %d is waiting to enter\n", getpid());
    //// PROBLEM HEREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
    // doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts
    // getShipTypeSems(sharedMemory, shipTypeSemIn, shipTypeSemPending, shipType);
    // printf("shipnode state: %d\n", shipNode->state);
    fprintf(logFileP, "Timestamp (millis): %lu -> Ship with pid %d and type %c entered the queue as incoming\n", (unsigned long)time(NULL), shipNode->shipId, shipNode->shipType);
    waitSemByShipType(sharedMemory, parkingSpotGroups, shipType);
    //     perror("sem_wait failed\n");
    // }
    // doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts

    // printf("vessel with pid: %d is entering\n", getpid());

    // doshifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts

    // ShipNode* currShipNode = &sharedMemory->shipNodes[sharedMemory->sizeIn - 1];
    if (shipNode->state == PendingEnter /*|| shipNode->state == PendingLeave*/) {
        // printf("hello1\n");
        fprintf(logFileP, "Timestamp (millis): %lu -> Ship with pid %d and type %c entered the pending queue\n", (unsigned long)time(NULL), shipNode->shipId, shipNode->shipType);
        waitSemPendingByShipType(sharedMemory, parkingSpotGroups, shipType);
    }
    // printf("hello\n");
    fprintf(logFileP, "Timestamp (millis): %lu -> Ship with pid %d and type %c entered the port\n", (unsigned long)time(NULL), shipNode->shipId, shipNode->shipType);

    usleep(manTimePeriodUsecs);  // sleep for manTimePeriod + parkTimePeriod

    // printf("vessel with pid: %d is posting inOutSem after enter ============================================================================, %c\n", getpid(), shipNode->shipType);
    sem_post(&sharedMemory->inOutSem);

    // postSemByShipType(sharedMemory, parkingSpotGroups, shipNode->shipType);
    // printf("HIIIIIIIIIIIIIII\n");
    shipNode->state = Parked;

    fprintf(logFileP, "Timestamp (millis): %lu -> Ship with pid %d and type %c parked in the port\n", (unsigned long)time(NULL), shipNode->shipId, shipNode->shipType);
    usleep(parkTimePeriodUsecs);  // here maybe ask the port-master for the cost ????????????????????????????????

    // shipNode->state = WaitingToLeave;
    // go to queue again
    // same shipNode pointer as above
    shipIndex = sharedMemory->sizeIn;

    shipNode->state = WaitingToLeave;
    shipNode = addShipNodeToShmByShipNode(sharedMemory, shipNodes, *shipNode);

    if (sem_post(&sharedMemory->portMasterWakeSem) < 0)
        perror("portMasterWakeSem failed");

    // printf("vessel with pid: %d is waiting to leave\n", getpid());

    fprintf(logFileP, "Timestamp (millis): %lu -> Ship with pid %d and type %c entered the queue as outgoing\n", (unsigned long)time(NULL), shipNode->shipId, shipNode->shipType);
    waitSemByShipType(sharedMemory, parkingSpotGroups, shipNode->shipType);

    usleep(manTimePeriodUsecs);
    // printf("vessel with pid: %d is posting inOutSem after leave ============================================================================\n", getpid());
    // doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts

    shipNode->state = Completed;
    sem_post(&sharedMemory->inOutSem);
    fprintf(logFileP, "Timestamp (millis): %lu -> Ship with pid %d and type %c left the port\n", (unsigned long)time(NULL), shipNode->shipId, shipNode->shipType);

    // maybe destroy local semaphores ???????

    fclose(logFileP);
    shmdt(sharedMemory);
    shmdt(shipNodes);
    shmdt(ledgerShipNodes);
    shmdt(publicLedger);
    shmdt(parkingSpotGroups);

    return 0;
}