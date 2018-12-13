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
                      LedgerShipNode** ledgerShipNodes, ShipNode** shipNodesIn, ShipNode** shipNodesOut, unsigned int* nextInIndex, unsigned int* nextOutIndex,
                      unsigned int* nextLedgerShipNodeIndex, ParkingSpotGroup* parkingSpotGroups[3]) {
    shipTypesSemIn = &sharedMemory->shipTypesSemIn;
    shipTypesSemOut = &sharedMemory->shipTypesSemOut;
    inOutSem = &sharedMemory->inOutSem;

    ledgerShipNodes = &sharedMemory->publicLedger->ledgerShipNodes;

    shipNodesIn = &sharedMemory->shipNodes;
    shipNodesOut = &sharedMemory->shipNodesOut;  // maybe "&" ????????

    nextInIndex = &sharedMemory->sizeIn;
    nextOutIndex = &sharedMemory->sizeOut;
    nextLedgerShipNodeIndex = &sharedMemory->publicLedger->size;
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
}

void handleIncomingShip(SharedMemory* sharedMemory, ShipNode** shipNodesIn, ParkingSpotGroup* parkingSpotGroups[3], unsigned int* sizePendingShipNodes,
                        sem_t* shipTypesSemIn[3], sem_t* inOutSem, unsigned int* nextInIndex, ShipNode* pendingShipNodeRequests[100]) {
    ShipNode* curShipNodeRequest = shipNodesIn[*nextInIndex];
    char curShipType = curShipNodeRequest->shipType;
    sem_t* curShipTypeSem = getShipTypeSem(parkingSpotGroups, shipTypesSemIn, curShipType);
    ParkingSpotGroup* curParkingSpotGroup = getShipParkingSpotGroup(parkingSpotGroups, curShipType);

    // check for available place
    if (curParkingSpotGroup->curCapacity == 0) {
        pendingShipNodeRequests[*sizePendingShipNodes] = &curShipNodeRequest;  // without "&" ???????????????????????
        (*sizePendingShipNodes)++;
    } else {
        curParkingSpotGroup->curCapacity--;
        LedgerShipNode* curLedgerShipNode = addLedgerShipNode(sharedMemory->publicLedger, curShipNodeRequest, curParkingSpotGroup);

        sem_wait(inOutSem);
        sem_post(curShipTypeSem);
    }

    (*nextInIndex)++;
}

void handleOutGoingShip(ShipNode** shipNodesOut, unsigned int* nextOutIndex, ParkingSpotGroup* parkingSpotGroups[3], sem_t* inOutSem, sem_t* shipTypesSemOut[3]) {
    // handle outgoing ship
    ShipNode* curShipNodeRequest = shipNodesOut[*nextOutIndex];
    char curShipType = curShipNodeRequest->shipType;
    sem_t* curShipTypeSem = getShipTypeSem(parkingSpotGroups, shipTypesSemOut, curShipType);
    ParkingSpotGroup* curParkingSpotGroup = getShipParkingSpotGroup(parkingSpotGroups, curShipType);
    LedgerShipNode* curLedgerShipNode = curShipNodeRequest->ledgerShipNode;

    sem_wait(inOutSem);
    sem_post(curShipTypeSem);
    usleep(curShipNodeRequest->manTimePeriod);

    curParkingSpotGroup->curCapacity++;
    curLedgerShipNode->state = Completed;
    (*nextOutIndex)++;
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
    unsigned int *sizeIn, *sizeOut, *ledgerSize;

    getValuesFromShm(sharedMemory, shipTypesSemIn, shipTypesSemOut, inOutSem, shmWriteSem, ledgerShipNodes,
                     shipNodesIn, shipNodesOut, sizeIn, sizeOut, ledgerSize, parkingSpotGroups);

    ShipNode* pendingShipNodeRequests[100];  // this will have something relevant with sizeOfShipNodes
    unsigned int nextInIndex = 0, nextOutIndex = 0, sizePendingShipNodes = 0, nextPendingShipNodeIndex = 0;
    // nextPendingShipNodeIndex: used for scanning
    // sizePendingShipNodes: used for insertion

    // wait for a ship to wake the port-master up !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // unsigned int sizeIn, sizeOut;
    while (nextInIndex < (*sizeIn) || nextOutIndex < (*sizeOut)) {
        // FIRST HANDLE PENDING REQUESTS <------------------ to be implemented

        if (nextInIndex < (*sizeIn)) {
            handleIncomingShip(sharedMemory, shipNodesIn, parkingSpotGroups, &sizePendingShipNodes,
                               shipTypesSemIn, inOutSem, &nextInIndex, pendingShipNodeRequests);
        }

        if (nextOutIndex < (*sizeOut)) {
            handleOutGoingShip(shipNodesOut, &nextOutIndex, parkingSpotGroups, inOutSem, shipTypesSemOut);
        }

        // hanlde pending ships
        // ShipNode* curShipNodeRequest = shipNodesOut[*nextOutIndex];
        // char curShipType = curShipNodeRequest->shipType;
        // sem_t* curShipTypeSem = getShipTypeSem(parkingSpotGroups, shipTypesSemOut, curShipType);
        // ParkingSpotGroup* curParkingSpotGroup = getShipParkingSpotGroup(parkingSpotGroups, curShipType);
        // LedgerShipNode* curLedgerShipNode = curShipNodeRequest->ledgerShipNode;

        // nextShipNodeRequestIndex++;
    }
}