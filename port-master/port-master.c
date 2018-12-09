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

void getValuesFromShm(SharedMemory* sharedMemory, sem_t* shipTypesSemIn[3], sem_t* shipTypesSemOut[3], sem_t* inOutSem, sem_t* shmWriteSem,
                      LedgerShipNode** ledgerShipNodes, ShipNode** shipNodesIn, ShipNode** shipNodesOut, unsigned int* nextShipNodeInIndex, unsigned int* nextShipNodeOutIndex,
                      unsigned int* nextLedgerShipNodeIndex, ParkingSpotGroup* parkingSpotGroups[3]) {
    shipTypesSemIn = &sharedMemory->shipTypesSemIn;
    shipTypesSemOut = &sharedMemory->shipTypesSemOut;
    inOutSem = &sharedMemory->inOutSem;

    ledgerShipNodes = &sharedMemory->publicLedger->ledgerShipNodes;

    shipNodesIn = &sharedMemory->shipNodes;
    shipNodesOut = &sharedMemory->shipNodesOut;  // maybe "&" ????????

    nextShipNodeInIndex = &sharedMemory->nextShipNodeIndex;
    nextShipNodeOutIndex = &sharedMemory->nextOutShipNodeIndex;
    nextLedgerShipNodeIndex = &sharedMemory->publicLedger->nextShipNodeIndex;
}

LedgerShipNode* addLedgerShipNode(PublicLedger* publicLedger, ShipNode* shipNode, ParkingSpotGroup* curParkingSpotGroup) {
    struct timeval curTime;
    gettimeofday(&curTime, NULL);

    unsigned int nextLedgerNodeIndex = publicLedger->nextShipNodeIndex;
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

    publicLedger->nextShipNodeIndex++;
}

void handleIncomingShip(ShipNode** shipNodesIn, unsigned int nextShipNodeRequestIndex, ParkingSpotGroup* parkingSpotGroups[3], unsigned int* nextPendingShipNodeRequestInsertIndex,
                        sem_t* shipTypesSemIn[3], sem_t inOutSem, unsigned int* nextShipNodeInIndex) {
    ShipNode* curShipNodeRequest = shipNodesIn[nextShipNodeRequestIndex];
    char curShipType = curShipNodeRequest->shipType;
    sem_t* curShipTypeSem = getShipTypeSem(parkingSpotGroups, shipTypesSemIn, curShipType);
    ParkingSpotGroup* curParkingSpotGroup = getShipParkingSpotGroup(parkingSpotGroups, curShipType);

    // check for available place
    if (curParkingSpotGroup->curCapacity == 0) {
        pendingShipNodeRequests[*nextPendingShipNodeRequestInsertIndex] = &curShipNodeRequest;  // without "&" ???????????????????????
        (*nextPendingShipNodeRequestInsertIndex)++;
    } else {
        curParkingSpotGroup->curCapacity--;
        LedgerShipNode* curLedgerShipNode = addLedgerShipNode(sharedMemory->publicLedger, curShipNodeRequest, curParkingSpotGroup);

        sem_wait(inOutSem);
        sem_post(curShipTypeSem);
    }

    (*nextShipNodeInIndex)++;
}

int main(int argc, char** argv) {
    int shmId;
    char* logFileName;
    handleFlags(argc, argv, &shmId, &logFileName);

    SharedMemory* sharedMemory = (SharedMemory*)shmat(shmId, NULL, 0);                           // error checking add <-------------------
    doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts

    // sem_t *shipTypesSemIn[3] = &sharedMemory->shipTypesSemIn, *shipTypesSemOut[3] = &sharedMemory->shipTypesSemOut, *inOutSem = &sharedMemory->inOutSem;
    // LedgerShipNode** ledgerShipNodes = sharedMemory->publicLedger->ledgerShipNodes;

    sem_t *shipTypesSemIn[3], *shipTypesSemOut[3], *inOutSem, *shmWriteSem;
    LedgerShipNode* ledgerShipNodes[3];
    ShipNode **shipNodesIn, **shipNodesOut;
    ParkingSpotGroup* parkingSpotGroups[3];
    unsigned int *nextShipNodeInInsertIndex, *nextShipNodeOutInsertIndex, *nextLedgerNodeIndex;

    getValuesFromShm(sharedMemory, shipTypesSemIn, shipTypesSemOut, inOutSem, shmWriteSem, ledgerShipNodes,
                     shipNodesIn, shipNodesOut, nextShipNodeInInsertIndex, nextShipNodeOutInsertIndex, nextLedgerNodeIndex, parkingSpotGroups);

    ShipNode* pendingShipNodeRequests[100];  // this will have something relevant with sizeOfShipNodes
    unsigned int nextShipNodeInIndex = 0, nextShipNodeOutIndex = 0, nextPendingShipNodeRequestInsertIndex = 0, nextPendingShipNodeRequestIndex = 0;
    // nextPendingShipNodeRequestIndex: used for scanning
    // nextPendingShipNodeRequestInsertIndex: used for insertion

    while (nextShipNodeInIndex < sharedMemory->sizeOfShipNodes && nextShipNodeOutIndex < sharedMemory->sizeOfShipNodes) {
        // FIRST HANDLE PENDING REQUESTS <------------------ to be implemented

        handleIncomingShip();

        // handle outgoing ship
        ShipNode* curShipNodeRequest = shipNodesOut[nextShipNodeOutIndex];
        char curShipType = curShipNodeRequest->shipType;
        sem_t* curShipTypeSem = getShipTypeSem(parkingSpotGroups, shipTypesSemOut, curShipType);
        ParkingSpotGroup* curParkingSpotGroup = getShipParkingSpotGroup(parkingSpotGroups, curShipType);
        LedgerShipNode* curLedgerShipNode = curShipNodeRequest->ledgerShipNode;

        curParkingSpotGroup->curCapacity++;

        sem_wait(inOutSem);
        sem_post(curShipTypeSem);
        usleep(curShipNodeRequest->manTimePeriod);

        curLedgerShipNode->state = Completed;
        nextShipNodeOutIndex++;

        // nextShipNodeRequestIndex++;
    }
}