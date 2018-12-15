#include "utils.h"

void initParkingSpotGroup(ParkingSpotGroup* parkingSpot, char type, unsigned int maxCapacity, float cost) {
    parkingSpot->maxCapacity = maxCapacity;
    parkingSpot->costPer30Min = cost;
    parkingSpot->type = type;
    parkingSpot->curCapacity = maxCapacity;
    // parkingSpot->occupied = 0;
}

void initPublicLedger(SharedMemory* sharedMemory, PublicLedger* publicLedger, ParkingSpotGroup* parkingSpotGroups, char* parkingSpotTypes,
                      unsigned int* capacities, float* costs, sem_t inOutSem, sem_t shipTypesSemIn[3], sem_t shipTypesSemOut[3], int sizeOfShipNodes,
                      int sizeOfLedgerShipNodes, sem_t shmWriteSem, sem_t portMasterWakeSem, int shmIdPublicLedger, int shmIdParkingGroups, int shmIdShipNodes, int shmIdLedgerNodes) {
    sharedMemory->inOutSem = inOutSem;
    sharedMemory->shmWriteSem = shmWriteSem;
    sharedMemory->sizeOfShipNodes = sizeOfShipNodes;
    sharedMemory->sizeOfLedgerShipNodes = sizeOfLedgerShipNodes;
    // maybe provide sizeOfOutShipNodes as well ....................
    sharedMemory->shipTypesSemIn[0] = shipTypesSemIn[0];
    sharedMemory->shipTypesSemIn[1] = shipTypesSemIn[1];
    sharedMemory->shipTypesSemIn[2] = shipTypesSemIn[2];

    sharedMemory->shipTypesSemPending[0] = shipTypesSemOut[0];
    sharedMemory->shipTypesSemPending[1] = shipTypesSemOut[1];
    sharedMemory->shipTypesSemPending[2] = shipTypesSemOut[2];

    sharedMemory->shmIdPublicLedger = shmIdPublicLedger;
    sharedMemory->shmIdParkingSpotGroups = shmIdParkingGroups;
    sharedMemory->shmIdShipNodes = shmIdShipNodes;
    sharedMemory->shmIdLedgerNodes = shmIdLedgerNodes;

    sharedMemory->portMasterWakeSem = portMasterWakeSem;

    // sharedMemory->sizeOut = 0;
    sharedMemory->sizeIn = 0;
    publicLedger->size = 0;
    printf("1\n");

    // sharedMemory->shipTypes[0] = parkingSpotTypes[0];
    // sharedMemory->shipTypes[1] = parkingSpotTypes[1];
    // sharedMemory->shipTypes[2] = parkingSpotTypes[2];

    for (unsigned int i = 0; i < 3; i++) {
        printf("i = %d\n", i);
        initParkingSpotGroup(&(parkingSpotGroups[i]), parkingSpotTypes[i], capacities[i], costs[i]);
    }
}

void doShifts(SharedMemory* sharedMemory, int sizeOfShipNodes, int sizeOfLedgerShipNodes) {
    printf("in doShifts: sizeOfShipNodes = %d, sizeOfLedgerNodes = %d\n", sizeOfShipNodes, sizeOfLedgerShipNodes);
    // sharedMemory = (SharedMemory*)((uint8_t*)sharedMemory);
    sharedMemory->publicLedger = (PublicLedger*)(((uint8_t*)sharedMemory) + sizeof(SharedMemory));

    sharedMemory->parkingSpotGroups = (ParkingSpotGroup*)(((uint8_t*)sharedMemory->publicLedger) + sizeof(PublicLedger));
    sharedMemory->shipNodes = (ShipNode*)(((uint8_t*)sharedMemory->parkingSpotGroups) + (3 * sizeof(ParkingSpotGroup)));
    sharedMemory->publicLedger->ledgerShipNodes = (LedgerShipNode*)(((uint8_t*)sharedMemory->shipNodes) + sizeOfShipNodes);  // sharedMemory->shipNodesOut = (ShipNode**)sharedMemory + sizeof(SharedMemory) + sizeof(PublicLedger) + 3 * sizeof(ParkingSpotGroup) + sizeOfShipNodes + sizeOfLedgerShipNodes;
}

void getShipTypeSems(SharedMemory* sharedMemory, ParkingSpotGroup* parkingSpotGroups, sem_t* shipTypeSem, sem_t* shipTypeSemPending, char shipType) {
    for (unsigned int i = 0; i < 3; i++) {
        if (shipType == parkingSpotGroups[i].type) {
            shipTypeSem = &sharedMemory->shipTypesSemIn[i];  /// maybe remove "&" ????????????????? here and below
            shipTypeSemPending = &sharedMemory->shipTypesSemPending[i];
            break;
        }
    }
}

void postSemByShipType(SharedMemory* sharedMemory, ParkingSpotGroup* parkingSpotGroups, char shipType) {
    for (unsigned int i = 0; i < 3; i++) {
        if (shipType == parkingSpotGroups[i].type) {
            sem_post(&sharedMemory->shipTypesSemIn[i]);  /// maybe remove "&" ????????????????? here and below
            break;
        }
    }
}

void waitSemByShipType(SharedMemory* sharedMemory, ParkingSpotGroup* parkingSpotGroups, char shipType) {
    printf("in waitSemByShipType\n");
    for (unsigned int i = 0; i < 3; i++) {
        if (shipType == parkingSpotGroups[i].type) {
            // doShifts(sharedMemory, sharedMemory->sizeOfShipNodes, sharedMemory->sizeOfLedgerShipNodes);  // do necessary shifts
            printf("vessel is waiting on type semaphore\n");
            sem_wait(&sharedMemory->shipTypesSemIn[i]);                                                  /// maybe remove "&" ????????????????? here and below
            break;
        }
    }
}

sem_t* getShipTypeSem(SharedMemory* sharedMemory, ParkingSpotGroup* parkingSpotGroups, char shipType) {
    for (unsigned int i = 0; i < 3; i++) {
        if (shipType == parkingSpotGroups[i].type) {
            return &sharedMemory->shipTypesSemIn[i];
        }
    }
    printf("Oops ship type sem get\n");
    return NULL;
}

ParkingSpotGroup* getShipParkingSpotGroup(ParkingSpotGroup parkingSpotGroups[3], char shipType) {
    printf("SHIP TYPE::::::::::::::::::::::::::::::::::::::::::::::: %c\n", shipType);
    for (unsigned int i = 0; i < 3; i++) {
        if (shipType == parkingSpotGroups[i].type) {
            return &parkingSpotGroups[i];
        }
    }
    printf("Oops parking spot group get\n");
    return NULL;
}

void execPortMaster(int shmId, char* logFileName) {
    char shmIdS[MAX_STRING_INT_SIZE];

    sprintf(shmIdS, "%d", shmId);

    char* args[] = {"executables/port-master", "-s", shmIdS, "-lf", logFileName, NULL};
    if (execvp(args[0], args) < 0) {
        printf("Exec port-master failed\n");
    }
}

void execMonitor(suseconds_t statusPrintTime, suseconds_t statsPrintTime, int shmId, char* logFileName) {
    char statusPrintTimeS[MAX_STRING_INT_SIZE], statsPrintTimeS[MAX_STRING_INT_SIZE], shmIdS[MAX_STRING_INT_SIZE];

    sprintf(statusPrintTimeS, "%ld", statusPrintTime);
    sprintf(statsPrintTimeS, "%ld", statsPrintTime);
    sprintf(shmIdS, "%d", shmId);

    char* args[] = {"executables/monitor", "-d", statusPrintTimeS, "-t", statsPrintTimeS, "-s", shmIdS, "-lf", logFileName, NULL};
    if (execvp(args[0], args) < 0) {
        printf("Exec monitor failed\n");
    }
}

void execVessel(char shipType, char upgradeFlag /* 0 or 1 */, long parkTime, long manTime, int shmId, char* logFileName) {
    char shipTypeS[2], upgradeFlagS[2], parkTimeS[MAX_STRING_INT_SIZE], manTimeS[MAX_STRING_INT_SIZE], shmIdS[MAX_STRING_INT_SIZE];

    sprintf(shipTypeS, "%c", shipType);
    sprintf(upgradeFlagS, "%c", upgradeFlag);
    sprintf(parkTimeS, "%ld", parkTime);
    sprintf(manTimeS, "%ld", manTime);
    sprintf(shmIdS, "%d", shmId);

    char* args[] = {"executables/vessel", "-t", shipTypeS, "-u", upgradeFlagS, "-p", parkTimeS, "-m", manTimeS, "-s", shmIdS, "-lf", logFileName, NULL};
    if (execvp(args[0], args) < 0) {
        printf("Exec vessel failed\n");
    }
}