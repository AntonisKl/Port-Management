#include "port-master.h"

void handleFlags(int argc, char** argv, /* char** chargesFileName,*/ int* shmId, char** logFileName) {
    if (argc != 5) {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    // char* ptr;

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

void getValuesFromShm(SharedMemory* sharedMemory, sem_t* shipTypesSemIn[3] /*, sem_t* shipTypesSemOut[3]*/, sem_t** inOutSem, sem_t** shmWriteSem, sem_t** portMasterWakeSem,
                      LedgerShipNode** ledgerShipNodes, ShipNode** shipNodesIn /*, ShipNode** shipNodesOut*/, unsigned int* nextInIndex /*, unsigned int* nextOutIndex*/,
                      unsigned int* nextLedgerShipNodeIndex, ParkingSpotGroup* parkingSpotGroups[3]) {
    printf("getting values from shm\n");
    // shipTypesSemIn = &sharedMemory->shipTypesSemIn;
    (shipTypesSemIn[0]) = &sharedMemory->shipTypesSemIn[0];
    (shipTypesSemIn[1]) = &sharedMemory->shipTypesSemIn[1];
    (shipTypesSemIn[2]) = &sharedMemory->shipTypesSemIn[2];
    // shipTypesSemOut = &sharedMemory->shipTypesSemOut;
    (*inOutSem) = &sharedMemory->inOutSem;
    (*shmWriteSem) = &sharedMemory->shmWriteSem;
    (*portMasterWakeSem) = &sharedMemory->portMasterWakeSem;

    ledgerShipNodes = &sharedMemory->publicLedger->ledgerShipNodes;

    shipNodesIn = &sharedMemory->shipNodes;
    // shipNodesOut = &sharedMemory->shipNodesOut;  // maybe "&" ????????

    (*nextInIndex) = sharedMemory->sizeIn;
    // printf("PORT-MASTER  sizeIn = %u\n", **nextInIndex);
    // nextOutIndex = &sharedMemory->sizeOut;
    (*nextLedgerShipNodeIndex) = sharedMemory->publicLedger->size;
}

State getNextPendingShipState(State state) {
    if (state == WaitingToEnter)
        return PendingEnter;
    else if (state == WaitingToLeave)
        return PendingLeave;
    else
        return state;  // this will not happen
}

LedgerShipNode* addLedgerShipNode(PublicLedger* publicLedger, ShipNode* shipNode, LedgerShipNode* ledgerShipNodes, ParkingSpotGroup* parkingSpotGroups) {
    struct timeval curTime;
    // printf("HI1\n");

    gettimeofday(&curTime, NULL);
    // printf("HI1.5\n");

    unsigned int nextLedgerNodeIndex = publicLedger->size;
    // printf("HI2\n");
    ledgerShipNodes[nextLedgerNodeIndex].shipId = shipNode->shipId;
    ledgerShipNodes[nextLedgerNodeIndex].upgradeFlag = shipNode->upgradeFlag;
    ledgerShipNodes[nextLedgerNodeIndex].shipType = shipNode->shipType;
    ledgerShipNodes[nextLedgerNodeIndex].arrivalTime = curTime.tv_usec;
    ledgerShipNodes[nextLedgerNodeIndex].parkTimePeriod = shipNode->parkTimePeriod;
    ledgerShipNodes[nextLedgerNodeIndex].manTimePeriod = shipNode->manTimePeriod;
    ledgerShipNodes[nextLedgerNodeIndex].parkingSpotGroupOccupied = &parkingSpotGroups[getShipTypeIndex(parkingSpotGroups, shipNode->shipType)];
    // improve cost mechanism
    ledgerShipNodes[nextLedgerNodeIndex].stayCost = ((shipNode->parkTimePeriod / 1000) / 30) * parkingSpotGroups[getShipTypeIndex(parkingSpotGroups, shipNode->shipType)].costPer30Min;  // handle minutes as milliseconds
    ledgerShipNodes[nextLedgerNodeIndex].state = shipNode->state;

    shipNode->ledgerShipNode = &ledgerShipNodes[nextLedgerNodeIndex];  /// point to ledger node

    publicLedger->size++;
    // printf("HI\n");
    return shipNode->ledgerShipNode;
}

void handleNextShip(SharedMemory* sharedMemory, ShipNode* shipNode, PublicLedger* publicLedger, LedgerShipNode* ledgerShipNodes,
                    ParkingSpotGroup* parkingSpotGroups, unsigned int* sizeOfPendingShipNodes,
                    unsigned int* nextIndex, ShipNode* pendingShipNodeRequests[100], FILE* logFileP) {
    // printf("PORT-MASTER: HELLLO 1\n");
    // doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts
    // printf("PORT-MASTER: HELLLO 2, nextIndex = %d\n", *nextIndex);
    // ShipNode* curShipNode = shipNodes[*nextIndex];
    // printf("PORT-MASTER: HELLLO\n");
    printf("PORT-MASTER: handling ship with pid: %d and state: %d and type: %c\n", shipNode->shipId, shipNode->state, shipNode->shipType);
    State curState = shipNode->state;
    printf("got state: %d\n", shipNode->state);
    if (curState == WaitingToEnter || curState == PendingEnter) {
        handleIncomingShip(sharedMemory, shipNode, parkingSpotGroups, publicLedger, ledgerShipNodes, nextIndex, pendingShipNodeRequests, sizeOfPendingShipNodes, 0, logFileP);
    } else if (curState == WaitingToLeave || curState == PendingLeave) {
		if (shipNode->ledgerShipNode == NULL)
			printf("NULLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\n");
        fprintf(logFileP, "Timestamp (millis): %lu -> Port master starts handling outgoing ship with pid %d\n", (unsigned long)time(NULL), shipNode->shipId);
        handleOutGoingShip(sharedMemory, shipNode, parkingSpotGroups, nextIndex, shipNode->ledgerShipNode->parkingSpotGroupOccupied->type == shipNode->shipType);
        fprintf(logFileP, "Timestamp (millis): %lu -> Port master finished handling outgoing ship with pid %d\n", (unsigned long)time(NULL), shipNode->shipId);
    }

    // (*nextIndex)++;
}

void handleOutGoingShip(SharedMemory* sharedMemory, ShipNode* shipNode, ParkingSpotGroup* parkingSpotGroups, unsigned int* nextIndex, char withUpgrade) {
    printf("PORT-MASTER: handling outgoing ship\n");

    char curShipType = shipNode->shipType;
    // sem_t *curShipTypeSem = SEM_FAILED, *curShipTypeSemPending = SEM_FAILED;
    // getShipTypeSems(sharedMemory, parkingSpotGroups, curShipTypeSem, curShipTypeSemPending, curShipType);
    unsigned int curShipTypeIndex;
    if (withUpgrade == 0)
        curShipTypeIndex = getShipTypeIndex(parkingSpotGroups, curShipType);
    else
        curShipTypeIndex = getShipTypeIndex(parkingSpotGroups, shipNode->upgradeFlag);

    LedgerShipNode* curLedgerShipNode = shipNode->ledgerShipNode;
    State curState = shipNode->state;

    // printf("PORT MASTER: waiting on inOutSem of outgoing ship ============================================================================\n");
    sem_wait(&sharedMemory->inOutSem);
    // doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts

    // printf("PORT-MASTER: outgoing ship handling after wait inOutSem\n");
    if (curState == WaitingToLeave)
        postSemByShipType(sharedMemory, parkingSpotGroups, curShipType);
    else if (curState == PendingLeave)
        postSemPendingByShipType(sharedMemory, parkingSpotGroups, curShipType);  /////////////////////////////////////////////////////////// HERE CHANGE IT TO A CUSTOM POST FUNCTION LIKE ABOVE
    // printf("PORT-MASTER: outgoing ship handling after post\n");

    usleep(shipNode->manTimePeriod);

    curLedgerShipNode->state = Completed;
    parkingSpotGroups[curShipTypeIndex].curCapacity++;
    printf("curCapacity = %u\n", parkingSpotGroups[curShipTypeIndex].curCapacity);

    (*nextIndex)++;
}

void handleIncomingShip(SharedMemory* sharedMemory, ShipNode* shipNode, ParkingSpotGroup* parkingSpotGroups, PublicLedger* publicLedger,
                        LedgerShipNode* ledgerShipNodes, unsigned int* nextIndex,
                        ShipNode* pendingShipNodeRequests[100], unsigned int* sizeOfPendingShipNodes, char withUpgrade, FILE* logFileP) {
    // printf("PORT MAASTER: handling incoming ship\n");
    // doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts

    // ShipNode* curShipNodeRequest = &shipNodesIn[*nextInIndex];

    char curShipType = shipNode->shipType;
	printf("HEREEEEEEEEEEEEEEEEEEEEEEEEEEE\n");
    printf("PORT-MASTER: handling incoming ship 2, withUpgrade = %d, type = %c, paking capacity = %u\n", withUpgrade, curShipType, parkingSpotGroups[0].maxCapacity);
    // sem_t *curShipTypeSem = SEM_FAILED, *curShipTypeSemPending = SEM_FAILED;
    // getShipTypeSems(sharedMemory, curShipTypeSem, curShipTypeSemPending, curShipType);
    unsigned int curShipTypeIndex;
    // ParkingSpotGroup* curParkingSpotGroup;
    if (withUpgrade == 0)
        curShipTypeIndex = getShipTypeIndex(parkingSpotGroups, curShipType);
    else if (withUpgrade == 1)
        curShipTypeIndex = getShipTypeIndex(parkingSpotGroups, shipNode->upgradeFlag);

    // printf("PORT MAASTER: handling incoming ship 2.5\n");

    State curState = shipNode->state;

    printf("PORT-MASTER: handling incoming ship 3 capacity: %u\n", parkingSpotGroups[curShipTypeIndex].curCapacity);
    // check for available place
    if (parkingSpotGroups[curShipTypeIndex].curCapacity == 0 && curState != PendingEnter) {
        if (withUpgrade == 0) {
            printf("HEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEELLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLOOOOOOOOOOOOOOOOO\n\n");
            fprintf(logFileP, "Timestamp (millis): %lu -> Port master starts handling incoming ship with pid %d\n", (unsigned long)time(NULL), shipNode->shipId);
        } else if (withUpgrade == 1)
            fprintf(logFileP, "Timestamp (millis): %lu -> Port master starts handling incoming ship with upgrade with pid %d\n", (unsigned long)time(NULL), shipNode->shipId);

        if (withUpgrade == 0 && (getShipTypeIndex(parkingSpotGroups, shipNode->upgradeFlag) > getShipTypeIndex(parkingSpotGroups, curShipType))) {
            handleIncomingShip(sharedMemory, shipNode, parkingSpotGroups, publicLedger, ledgerShipNodes, nextIndex, pendingShipNodeRequests,
                               sizeOfPendingShipNodes, 1, logFileP);
            return;
        }

        shipNode->state = getNextPendingShipState(curState);
        // printf("PORT MAASTER: handling incoming ship 4a\n");

        pendingShipNodeRequests[*sizeOfPendingShipNodes] = shipNode;  // without "&" ???????????????????????
        (*sizeOfPendingShipNodes)++;

        postSemByShipType(sharedMemory, parkingSpotGroups, curShipType);  // vessel will then wait for pending semapohore
        (*nextIndex)++;
        if (withUpgrade == 0)
            fprintf(logFileP, "Timestamp (millis): %lu -> Port master finished handling incoming ship with pid %d\n", (unsigned long)time(NULL), shipNode->shipId);
        else if (withUpgrade == 1)
            fprintf(logFileP, "Timestamp (millis): %lu -> Port master finished handling incoming ship with upgrade with pid %d\n", (unsigned long)time(NULL), shipNode->shipId);
    } else if (parkingSpotGroups[curShipTypeIndex].curCapacity > 0) {
        if (curState == WaitingToEnter) {
            if (withUpgrade == 0)
                fprintf(logFileP, "Timestamp (millis): %lu -> Port master starts handling incoming ship with pid %d\n", (unsigned long)time(NULL), shipNode->shipId);
            else if (withUpgrade == 1)
                fprintf(logFileP, "Timestamp (millis): %lu -> Port master starts handling incoming ship with upgrade with pid %d\n", (unsigned long)time(NULL), shipNode->shipId);
        } else if (curState == PendingEnter) {
            if (withUpgrade == 0)
                fprintf(logFileP, "Timestamp (millis): %lu -> Port master starts handling pending incoming ship with pid %d\n", (unsigned long)time(NULL), shipNode->shipId);
            else if (withUpgrade == 1)
                fprintf(logFileP, "Timestamp (millis): %lu -> Port master starts handling pending incoming ship with upgrade with pid %d\n", (unsigned long)time(NULL), shipNode->shipId);
        }
        parkingSpotGroups[curShipTypeIndex].curCapacity--;
        // doshifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts
        LedgerShipNode* curLedgerShipNode = addLedgerShipNode(publicLedger, shipNode, ledgerShipNodes, parkingSpotGroups);

        // printf("PORT MAASTER: handling incoming ship 4 waiting on inOutSem (incoming ship) ============================================================================\n");
        // doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts

        sem_wait(&sharedMemory->inOutSem);
        // doshifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts
        // printf("PORT MAASTER: after wait of inOutSem, vessel's state = %d\n", curState);
        if (curState == WaitingToEnter) {
            // printf("PORT MAASTER: before post vessel\n");
            // sem_post(&sharedMemory->shipTypesSemIn[0]);
            postSemByShipType(sharedMemory, parkingSpotGroups, curShipType);
            if (withUpgrade == 0)
                fprintf(logFileP, "Timestamp (millis): %lu -> Port master finished handling incoming ship with pid %d\n", (unsigned long)time(NULL), shipNode->shipId);
            else if (withUpgrade == 1)
                fprintf(logFileP, "Timestamp (millis): %lu -> Port master finished handling incoming ship with upgrade with pid %d\n", (unsigned long)time(NULL), shipNode->shipId);
            // printf("PORT MAASTER: after post vessel\n");
        } else if (curState == PendingEnter) {
            postSemPendingByShipType(sharedMemory, parkingSpotGroups, curShipType);
            if (withUpgrade == 0)
                fprintf(logFileP, "Timestamp (millis): %lu -> Port master finished handling pending incoming ship with pid %d\n", (unsigned long)time(NULL), shipNode->shipId);
            else if (withUpgrade == 1)
                fprintf(logFileP, "Timestamp (millis): %lu -> Port master finished handling pending incoming ship with upgrade with pid %d\n", (unsigned long)time(NULL), shipNode->shipId);
        }
        (*nextIndex)++;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //  else if (parkingSpotGroups[curShipTypeIndex].curCapacity == 0 && curState == PendingEnter) {                                          //
    //     usleep(1000000); // sleep for 1 second/////////////////////////////////////////////////////////// MAYBE NEEDED //////////////////////
    // }                                                                                                                                      //
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

// void handleOutGoingShip(ShipNode** shipNodesOut, unsigned int* nextOutIndex, ParkingSpotGroup* parkingSpotGroups[3], sem_t* inOutSem, sem_t* shipTypesSemOut[3]) {
//     // handle outgoing ship
//     ShipNode* curShipNodeRequest = shipNodesOut[*nextOutIndex];
//     char curShipType = curShipNodeRequest->shipType;
//     sem_t* curShipTypeSem = getShipTypeSem(parkingSpotGroups, shipTypesSemOut, curShipType);
//     ParkingSpotGroup* curParkingSpotGroup = getShipParkingSpotGroup(parkingSpotGroups, curShipType);
//     LedgerShipNode* curLedgerShipNode = curShipNodeRequest->ledgerShipNode;

//     sem_wait(inOutSem);
//     sem_post(curShipTypeSem);
//     usleep(curShipNodeRequest->manTimePeriod);

//     curParkingSpotGroup->curCapacity++;
//     curLedgerShipNode->state = Completed;
//     (*nextOutIndex)++;
// }

int main(int argc, char** argv) {
    int shmId;
    char* logFileName;
    handleFlags(argc, argv, &shmId, &logFileName);
    // printf("PORT MAASTER handled flags\n");

    SharedMemory* sharedMemory = (SharedMemory*)((uint8_t*)shmat(shmId, 0, 0));  // error checking add <-------------------

    PublicLedger* publicLedger = (PublicLedger*)((uint8_t*)shmat(sharedMemory->shmIdPublicLedger, 0, 0));
    ParkingSpotGroup* parkingSpotGroups = (ParkingSpotGroup*)((uint8_t*)shmat(sharedMemory->shmIdParkingSpotGroups, 0, 0));
    ShipNode* shipNodes = (ShipNode*)((uint8_t*)shmat(sharedMemory->shmIdShipNodes, 0, 0));
    LedgerShipNode* ledgerShipNodes = (LedgerShipNode*)((uint8_t*)shmat(sharedMemory->shmIdLedgerNodes, 0, 0));

    // sharedMemory->publicLedger = (PublicLedger*)((uint8_t*)shmat(sharedMemory->shmIdPublicLedger, 0, 0));
    // sharedMemory->parkingSpotGroups = (ParkingSpotGroup*)((uint8_t*)shmat(sharedMemory->shmIdParkingSpotGroups, 0, 0));
    // sharedMemory->shipNodes = (ShipNode*)((uint8_t*)shmat(sharedMemory->shmIdShipNodes, 0, 0));
    // sharedMemory->publicLedger->ledgerShipNodes = (LedgerShipNode*)((uint8_t*)shmat(sharedMemory->shmIdLedgerNodes, 0, 0));

    // printf("port master: shm address: %p\n", (void*)&sharedMemory);
    // doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts

    // printf("PORT MAASTER shifted shm structs\n");
    // sharedMemory->shipNodes[0].shipType = 1;

    // sem_t *shipTypesSemIn[3] = &sharedMemory->shipTypesSemIn, *shipTypesSemOut[3] = &sharedMemory->shipTypesSemOut, *inOutSem = &sharedMemory->inOutSem;
    // LedgerShipNode** ledgerShipNodes = sharedMemory->publicLedger->ledgerShipNodes;

    // sem_t *shipTypesSemIn[3] /*, *shipTypesSemOut[3]*/, *inOutSem = SEM_FAILED, *shmWriteSem = SEM_FAILED, *portMasterWakeSem = SEM_FAILED;
    // LedgerShipNode* ledgerShipNodes[3];
    // ShipNode** shipNodesIn = NULL /*, **shipNodesOut*/;
    // ParkingSpotGroup* parkingSpotGroups[3];
    // unsigned int sizeIn = 0 /*, *sizeOut*/, ledgerSize = 0;
    unsigned int sizeIn = 0;

    // getValuesFromShm(sharedMemory, &shipTypesSemIn[3], &inOutSem, &shmWriteSem, &portMasterWakeSem, ledgerShipNodes,
    //                  shipNodesIn, &sizeIn, &ledgerSize, parkingSpotGroups);
    // printf("PORT MAASTER got values from shm\n");
    ShipNode* pendingShipNodeRequests[100];  // this will have something relevant with sizeOfShipNodes
    unsigned int nextInIndex = 0, nextPendingIndex = 0, sizeOfPendingShipNodes = 0;
    // nextPendingShipNodeIndex: used for scanning
    // sizeOfPendingShipNodes: used for insertion
    FILE* logFileP = fopen(logFileName, "a");

    char didSomething = 1;
    while (1) {
        // doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts

        printf("PORT MAASTER waiting for vessels\n");
        if (didSomething == 1) {
            fprintf(logFileP, "Timestamp (millis): %lu -> Port master is waiting for vessels\n", (unsigned long)time(NULL));
            didSomething = 0;
        }
        if (fclose(logFileP) == EOF) {
            perror("fclose failed");
        }
        sem_wait(&sharedMemory->portMasterWakeSem);

        if ((logFileP = fopen(logFileName, "a")) == NULL) {
            perror("fopen failed");
        }

        printf("PORT-MASTER woke up\n");

        // doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts

        // unsigned int sizeIn, sizeOut;
        sizeIn = sharedMemory->sizeIn;

        // ledgerSize = sharedMemory->publicLedger->size; // ??????????

        printf("starting PORT-MASTER while loop, sizeIn = %u, nextIndex = %u, sizeOfPendingShipNodes = %u, nextPendingIndex = %u\n", sizeIn, nextInIndex, sizeOfPendingShipNodes, nextPendingIndex);
        while (nextInIndex < sizeIn || nextPendingIndex < sizeOfPendingShipNodes /*|| nextOutIndex < (*sizeOut)*/) {
            // FIRST HANDLE PENDING REQUESTS <------------------ to be implemented

            // handleIncomingShip(sharedMemory, shipNodesIn, parkingSpotGroups, &sizeOfPendingShipNodes,
            //                    shipTypesSemIn, inOutSem, &nextInIndex, pendingShipNodeRequests);

            // ShipNode curShipNode = sharedMemory->shipNodes[nextInIndex];

            unsigned int prevNextIndex = nextInIndex, prevNextPendingIndex = nextPendingIndex;

            if (nextPendingIndex < sizeOfPendingShipNodes) {
                State curState = pendingShipNodeRequests[nextPendingIndex]->state;

                if (curState == PendingEnter || curState == PendingLeave) {
                    printf("2nd\n");
                    handleNextShip(sharedMemory, pendingShipNodeRequests[nextPendingIndex], publicLedger, ledgerShipNodes, parkingSpotGroups, NULL, &nextPendingIndex, NULL, logFileP);
                }
            }

            sizeIn = sharedMemory->sizeIn;
            printf("nextIndex = %u, sizeIn = %u\n", nextInIndex, sizeIn);
            if (nextInIndex < sizeIn) {
                // printf("1\n");
                // doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts
                State curState = shipNodes[nextInIndex].state;
                // printf("2\n");
                printf("PORT-MASTER: state of current vessel: %d\n", curState);
                if (curState == WaitingToEnter || curState == WaitingToLeave) {
                    printf("1st, ship type = %c\n", shipNodes[nextInIndex].shipType);
                    handleNextShip(sharedMemory, &shipNodes[nextInIndex], publicLedger, ledgerShipNodes, parkingSpotGroups,
                                   &sizeOfPendingShipNodes, &nextInIndex, pendingShipNodeRequests, logFileP);
                }
            }

            didSomething = 1;

            if (prevNextIndex == nextInIndex && prevNextPendingIndex == nextPendingIndex) {
                break;
            }

            // else if (curState == PendingEnter || curState == PendingLeave) {
            //     printf("2nd\n");
            //     handleNextShip(sharedMemory, pendingShipNodeRequests[nextPendingIndex], publicLedger, ledgerShipNodes, parkingSpotGroups, NULL, &nextPendingIndex, NULL);
            // }
            // else {
            //     nextInIndex++;  /////////////           ???????????????????????????????????
            // }
            // if (nextOutIndex < (*sizeOut)) {
            //     handleOutGoingShip(shipNodesOut, &nextOutIndex, parkingSpotGroups, inOutSem, shipTypesSemOut);
            // }

            // hanlde pending ships
            // ShipNode* curShipNodeRequest = shipNodesOut[*nextOutIndex];
            // char curShipType = curShipNodeRequest->shipType;
            // sem_t* curShipTypeSem = getShipTypeSem(parkingSpotGroups, shipTypesSemOut, curShipType);
            // ParkingSpotGroup* curParkingSpotGroup = getShipParkingSpotGroup(parkingSpotGroups, curShipType);
            // LedgerShipNode* curLedgerShipNode = curShipNodeRequest->ledgerShipNode;

            // nextShipNodeRequestIndex++;
            // nextInIndex++;
        }
    }

    fclose(logFileP);
    shmdt(sharedMemory);
    shmdt(shipNodes);
    shmdt(ledgerShipNodes);
    shmdt(publicLedger);
    shmdt(parkingSpotGroups);
}
