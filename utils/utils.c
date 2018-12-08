#include "utils.h"

void initParkingSpotGroup(ParkingSpotGroup* parkingSpot, char type, unsigned int maxCapacity, float cost) {
    parkingSpot->maxCapacity = maxCapacity;
    parkingSpot->costPer30Min = cost;
    parkingSpot->type = type;
    parkingSpot->curCapacity = maxCapacity;
    // parkingSpot->occupied = 0;
}

void initPublicLedger(SharedMemory* sharedMemory, char* parkingSpotTypes, unsigned int* capacities, float* costs, sem_t inOutSem, sem_t shipTypesSem[3], int sizeOfShipNodes, int sizeOfLedgerShipNodes) {
    sharedMemory->inOutSem = inOutSem;
    sharedMemory->sizeOfShipNodes = sizeOfShipNodes;
    sharedMemory->sizeOfLedgerShipNodes = sizeOfLedgerShipNodes;

    sharedMemory->shipTypesSem[0] = shipTypesSem[0];
    sharedMemory->shipTypesSem[1] = shipTypesSem[1];
    sharedMemory->shipTypesSem[2] = shipTypesSem[2];

    sharedMemory->nextShipNodeIndex = 0;
    sharedMemory->publicLedger->nextShipNodeIndex = 0;

    // sharedMemory->shipTypes[0] = parkingSpotTypes[0];
    // sharedMemory->shipTypes[1] = parkingSpotTypes[1];
    // sharedMemory->shipTypes[2] = parkingSpotTypes[2];

    for (unsigned int i = 0; i < 3; i++) {
        initParkingSpotGroup(&(sharedMemory->parkingSpotGroups[i]), parkingSpotTypes[i], capacities[i], costs[i]);
    }
}

void doShifts(SharedMemory* sharedMemory, int sizeOfShipNodes) {
    sharedMemory->publicLedger = (PublicLedger*)sharedMemory + sizeof(SharedMemory);
    sharedMemory->parkingSpotGroups = (ParkingSpotGroup*)sharedMemory + sizeof(SharedMemory) + sizeof(PublicLedger);
    sharedMemory->shipNodes = (ShipNode*)sharedMemory + sizeof(SharedMemory) + sizeof(PublicLedger) + 3 * sizeof(ParkingSpotGroup);
    sharedMemory->publicLedger->ledgerShipNodes = (LedgerShipNode*)sharedMemory + sizeof(SharedMemory) + sizeof(PublicLedger) + 3 * sizeof(ParkingSpotGroup) + sizeOfShipNodes;
}

void execPortMaster(int shmId, char* logFileName) {
    char shmIdS[MAX_STRING_INT_SIZE];

    sprintf(shmIdS, "%d", shmId);

    char* args[] = {"executables/port-master", "-s", shmIdS, "-lf", logFileName};
    if (execvp(args[0], args) < 0) {
        printf("Exec port-master failed\n");
    }
}

void execMonitor(suseconds_t statusPrintTime, suseconds_t statsPrintTime, int shmId, char* logFileName) {
    char statusPrintTimeS[MAX_STRING_INT_SIZE], statsPrintTimeS[MAX_STRING_INT_SIZE], shmIdS[MAX_STRING_INT_SIZE];

    sprintf(statusPrintTimeS, "%ld", statusPrintTime);
    sprintf(statsPrintTimeS, "%ld", statsPrintTime);
    sprintf(shmIdS, "%d", shmId);

    char* args[] = {"executables/port-master", "-d", statusPrintTimeS, "-t", statsPrintTimeS, "-s", shmIdS, "-lf", logFileName};
    if (execvp(args[0], args) < 0) {
        printf("Exec port-master failed\n");
    }
}

void execVessel(char shipType, char upgradeFlag /* 0 or 1 */, suseconds_t parkTime, suseconds_t manTime, int shmId, char* logFileName) {
    char shipTypeS[2], upgradeFlagS[2], parkTimeS[MAX_STRING_INT_SIZE], manTimeS[MAX_STRING_INT_SIZE], shmIdS[MAX_STRING_INT_SIZE];

    sprintf(shipTypeS, "%c", shipType);
    sprintf(upgradeFlagS, "%c", upgradeFlag);
    sprintf(parkTimeS, "%ld", parkTime);
    sprintf(manTimeS, "%ld", manTime);
    sprintf(shmIdS, "%d", shmId);

    char* args[] = {"executables/port-master", "-t", shipTypeS, "-u", upgradeFlagS, "-p", parkTimeS, "-m", manTimeS, "-s", shmIdS, "-lf", logFileName};
    if (execvp(args[0], args) < 0) {
        printf("Exec port-master failed\n");
    }
}