#include "utils.h"

void initParkingSpot(ParkingSpot* parkingSpot, char type, unsigned int capacity, float cost) {
    parkingSpot->capacity = capacity;
    parkingSpot->costPer30Min = cost;
    parkingSpot->parkingSpotType = type;
    parkingSpot->occupied = 0;
}

void initPublicLedger(PublicLedger* publicLedger, char* parkingSpotTypes, unsigned int* capacities, float* costs) {
    for (unsigned int i = 0; i < 3; i++) {
        initParkingSpot(&(publicLedger->parkingSpots[i]), parkingSpotTypes[i], capacities[i], costs[i]);
    }
}

void doShifts(PublicLedger* publicLedger) {
    publicLedger->parkingSpots = (ParkingSpot*)publicLedger + sizeof(PublicLedger);
    publicLedger->shipNodes = (ShipNode*)publicLedger + sizeof(PublicLedger) + 3 * sizeof(ParkingSpot);
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