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

void getValuesFromShm(SharedMemory* sharedMemory, sem_t* shipTypesSemIn[3] /*, sem_t* shipTypesSemOut[3]*/, sem_t* inOutSem, sem_t* shmWriteSem,
                      LedgerShipNode** ledgerShipNodes, ShipNode** shipNodesIn /*, ShipNode** shipNodesOut*/, unsigned int* nextInIndex /*, unsigned int* nextOutIndex*/,
                      unsigned int* nextLedgerShipNodeIndex, ParkingSpotGroup* parkingSpotGroups[3]) {
    // shipTypesSemIn = &sharedMemory->shipTypesSemIn;
    shipTypesSemIn[0] = &sharedMemory->shipTypesSemIn[0];
    shipTypesSemIn[1] = &sharedMemory->shipTypesSemIn[1];
    shipTypesSemIn[2] = &sharedMemory->shipTypesSemIn[2];
    // shipTypesSemOut = &sharedMemory->shipTypesSemOut;
    inOutSem = &sharedMemory->inOutSem;

    ledgerShipNodes = &sharedMemory->publicLedger->ledgerShipNodes;

    shipNodesIn = &sharedMemory->shipNodes;
    // shipNodesOut = &sharedMemory->shipNodesOut;  // maybe "&" ????????

    nextInIndex = &sharedMemory->sizeIn;
    // nextOutIndex = &sharedMemory->sizeOut;
    nextLedgerShipNodeIndex = &sharedMemory->publicLedger->size;
}

State getNextPendingShipState(State state) {
    if (state == WaitingToEnter)
        return PendingEnter;
    else if (state == WaitingToLeave)
        return PendingLeave;
    else
        return state;  // this will not happen
}

LedgerShipNode* addLedgerShipNode(PublicLedger* publicLedger, ShipNode* shipNode, ParkingSpotGroup* curParkingSpotGroup) {
    struct timeval curTime;
    gettimeofday(&curTime, NULL);

    unsigned int nextLedgerNodeIndex = publicLedger->size;
    publicLedger->ledgerShipNodes[nextLedgerNodeIndex].shipId = shipNode->shipId;
    publicLedger->ledgerShipNodes[nextLedgerNodeIndex].upgradeFlag = shipNode->upgradeFlag;
    publicLedger->ledgerShipNodes[nextLedgerNodeIndex].shipType = shipNode->shipType;
    publicLedger->ledgerShipNodes[nextLedgerNodeIndex].arrivalTime = curTime.tv_usec;
    publicLedger->ledgerShipNodes[nextLedgerNodeIndex].parkTimePeriod = shipNode->parkTimePeriod;
    publicLedger->ledgerShipNodes[nextLedgerNodeIndex].manTimePeriod = shipNode->manTimePeriod;
    publicLedger->ledgerShipNodes[nextLedgerNodeIndex].parkingSpotGroupOccupied = curParkingSpotGroup;
    // improve cost mechanism
    publicLedger->ledgerShipNodes[nextLedgerNodeIndex].stayCost = ((shipNode->parkTimePeriod / 1000) / 30) * curParkingSpotGroup->costPer30Min;  // handle minutes as milliseconds
    publicLedger->ledgerShipNodes[nextLedgerNodeIndex].state = shipNode->state;

    shipNode->ledgerShipNode = &publicLedger->ledgerShipNodes[nextLedgerNodeIndex];  /// point to ledger node

    publicLedger->size++;

    return shipNode->ledgerShipNode;
}

void handleNextShip(SharedMemory* sharedMemory, ShipNode** shipNodes, unsigned int* sizeOfPendingShipNodes,
                    unsigned int* nextIndex, ShipNode* pendingShipNodeRequests[100]) {
    ShipNode* curShipNode = shipNodes[*nextIndex];
    State curState = curShipNode->state;
    if (curState == WaitingToEnter || curState == PendingEnter) {
        handleIncomingShip(sharedMemory, curShipNode, nextIndex, pendingShipNodeRequests, sizeOfPendingShipNodes);
    } else if (curState == WaitingToLeave || curState == PendingLeave) {
        handleOutGoingShip(sharedMemory, curShipNode, nextIndex);
    }

    (*nextIndex)++;
}

void handleOutGoingShip(SharedMemory* sharedMemory, ShipNode* shipNode, unsigned int* nextIndex) {
    char curShipType = shipNode->shipType;
    sem_t *curShipTypeSem = SEM_FAILED, *curShipTypeSemPending = SEM_FAILED;
    getShipTypeSems(sharedMemory, curShipTypeSem, curShipTypeSemPending, curShipType);
    ParkingSpotGroup* curParkingSpotGroup = getShipParkingSpotGroup(sharedMemory->parkingSpotGroups, curShipType);
    LedgerShipNode* curLedgerShipNode = shipNode->ledgerShipNode;
    State curState = shipNode->state;

    sem_wait(&sharedMemory->inOutSem);

    if (curState == WaitingToLeave)
        sem_post(curShipTypeSem);
    else if (curState == PendingLeave)
        sem_post(curShipTypeSemPending);

    usleep(shipNode->manTimePeriod);

    curLedgerShipNode->state = Completed;
    curParkingSpotGroup->curCapacity++;
    // (*nextIndex)++;
}

void handleIncomingShip(SharedMemory* sharedMemory, ShipNode* shipNode, unsigned int* nextIndex,
                        ShipNode* pendingShipNodeRequests[100], unsigned int* sizeOfPendingShipNodes) {
    // ShipNode* curShipNodeRequest = &shipNodesIn[*nextInIndex];
    char curShipType = shipNode->shipType;
    sem_t *curShipTypeSem = SEM_FAILED, *curShipTypeSemPending = SEM_FAILED;
    getShipTypeSems(sharedMemory, curShipTypeSem, curShipTypeSemPending, curShipType);
    ParkingSpotGroup* curParkingSpotGroup = getShipParkingSpotGroup(sharedMemory->parkingSpotGroups, curShipType);
    State curState = shipNode->state;
    // check for available place
    if (curParkingSpotGroup->curCapacity == 0 && curState != PendingEnter) {
        shipNode->state = getNextPendingShipState(curState);

        pendingShipNodeRequests[*sizeOfPendingShipNodes] = shipNode;  // without "&" ???????????????????????
        (*sizeOfPendingShipNodes)++;

        sem_post(curShipTypeSem);  // vessel will then wait for pending semapohore
    } else if (curParkingSpotGroup->curCapacity > 0) {
        curParkingSpotGroup->curCapacity--;
        LedgerShipNode* curLedgerShipNode = addLedgerShipNode(sharedMemory->publicLedger, shipNode, curParkingSpotGroup);

        sem_wait(&sharedMemory->inOutSem);
        if (curState == WaitingToEnter)
            sem_post(curShipTypeSem);
        else if (curState == PendingEnter)
            sem_post(curShipTypeSemPending);
    }
    // (*nextIndex)++;
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

    SharedMemory* sharedMemory = (SharedMemory*)shmat(shmId, NULL, 0);                           // error checking add <-------------------
    doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts

    // sem_t *shipTypesSemIn[3] = &sharedMemory->shipTypesSemIn, *shipTypesSemOut[3] = &sharedMemory->shipTypesSemOut, *inOutSem = &sharedMemory->inOutSem;
    // LedgerShipNode** ledgerShipNodes = sharedMemory->publicLedger->ledgerShipNodes;

    sem_t *shipTypesSemIn[3] /*, *shipTypesSemOut[3]*/, *inOutSem = SEM_FAILED, *shmWriteSem = SEM_FAILED;
    LedgerShipNode* ledgerShipNodes[3];
    ShipNode** shipNodesIn = NULL /*, **shipNodesOut*/;
    ParkingSpotGroup* parkingSpotGroups[3];
    unsigned int *sizeIn = NULL /*, *sizeOut*/, *ledgerSize = NULL;

    getValuesFromShm(sharedMemory, shipTypesSemIn, inOutSem, shmWriteSem, ledgerShipNodes,
                     shipNodesIn, sizeIn, ledgerSize, parkingSpotGroups);

    ShipNode* pendingShipNodeRequests[100];  // this will have something relevant with sizeOfShipNodes
    unsigned int nextInIndex = 0, nextPendingIndex = 0, sizeOfPendingShipNodes = 0;
    // nextPendingShipNodeIndex: used for scanning
    // sizeOfPendingShipNodes: used for insertion

    // wait for a ship to wake the port-master up !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // unsigned int sizeIn, sizeOut;
    while (nextInIndex < (*sizeIn) || nextPendingIndex < sizeOfPendingShipNodes /*|| nextOutIndex < (*sizeOut)*/) {
        // FIRST HANDLE PENDING REQUESTS <------------------ to be implemented

        // handleIncomingShip(sharedMemory, shipNodesIn, parkingSpotGroups, &sizeOfPendingShipNodes,
        //                    shipTypesSemIn, inOutSem, &nextInIndex, pendingShipNodeRequests);

        // ShipNode curShipNode = sharedMemory->shipNodes[nextInIndex];
        State curState = sharedMemory->shipNodes[nextInIndex].state;
        if (curState == WaitingToEnter || curState == WaitingToLeave) {
            handleNextShip(sharedMemory, shipNodesIn, &sizeOfPendingShipNodes, &nextInIndex, pendingShipNodeRequests);
        } else if (curState == PendingEnter || curState == PendingLeave) {
            handleNextShip(sharedMemory, pendingShipNodeRequests, NULL, &nextPendingIndex, NULL);
        }
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
    }
}