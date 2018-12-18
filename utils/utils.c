#include "utils.h"

void initParkingSpotGroup(ParkingSpotGroup* parkingSpot, char type, unsigned int maxCapacity, float cost) {
    parkingSpot->maxCapacity = maxCapacity;
    parkingSpot->costPer30Millis = cost;
    parkingSpot->type = type;
    parkingSpot->curCapacity = maxCapacity;
    // parkingSpot->occupied = 0;
}

void initSharedUtils(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char* parkingSpotTypes,
                      unsigned int* capacities, float* costs, sem_t inOutSem, sem_t vesselTypesSemIn[3], sem_t vesselTypesSemOut[3], int sizeOfVesselNodes,
                      int sizeOfLedgerVesselNodes, sem_t shmWriteSem, sem_t portMasterWakeSem, int shmIdParkingGroups, int shmIdVesselNodes, int shmIdLedgerNodes) {
    sharedUtils->inOutSem = inOutSem;
    sharedUtils->shmWriteSem = shmWriteSem;
    sharedUtils->sizeOfVesselNodes = sizeOfVesselNodes;
    sharedUtils->sizeOfLedgerVesselNodes = sizeOfLedgerVesselNodes;

    sharedUtils->vesselTypesSemIn[0] = vesselTypesSemIn[0];
    sharedUtils->vesselTypesSemIn[1] = vesselTypesSemIn[1];
    sharedUtils->vesselTypesSemIn[2] = vesselTypesSemIn[2];

    sharedUtils->vesselTypesSemPending[0] = vesselTypesSemOut[0];
    sharedUtils->vesselTypesSemPending[1] = vesselTypesSemOut[1];
    sharedUtils->vesselTypesSemPending[2] = vesselTypesSemOut[2];

    sharedUtils->shmIdParkingSpotGroups = shmIdParkingGroups;
    sharedUtils->shmIdVesselNodes = shmIdVesselNodes;
    sharedUtils->shmIdLedgerNodes = shmIdLedgerNodes;

    sharedUtils->portMasterWakeSem = portMasterWakeSem;

    // sharedUtils->sizeOut = 0;
    sharedUtils->queueSize = 0;
    sharedUtils->ledgerSize = 0;
    // printf("1\n");

    // sharedUtils->vesselTypes[0] = parkingSpotTypes[0];
    // sharedUtils->vesselTypes[1] = parkingSpotTypes[1];
    // sharedUtils->vesselTypes[2] = parkingSpotTypes[2];

    for (unsigned int i = 0; i < 3; i++) {
        // printf("i = %d\n", i);
        initParkingSpotGroup(&(parkingSpotGroups[i]), parkingSpotTypes[i], capacities[i], costs[i]);
    }
}

// void doShifts(SharedUtils* sharedUtils, int sizeOfVesselNodes, int sizeOfLedgerVesselNodes) {
//     printf("in doShifts: sizeOfVesselNodes = %d, sizeOfLedgerNodes = %d\n", sizeOfVesselNodes, sizeOfLedgerVesselNodes);
//     // sharedUtils = (SharedUtils*)((uint8_t*)sharedUtils);
//     sharedUtils->publicLedger = (PublicLedger*)(((uint8_t*)sharedUtils) + sizeof(SharedUtils));

//     sharedUtils->parkingSpotGroups = (ParkingSpotGroup*)(((uint8_t*)sharedUtils->publicLedger) + sizeof(PublicLedger));
//     sharedUtils->vesselNodes = (VesselNode*)(((uint8_t*)sharedUtils->parkingSpotGroups) + (3 * sizeof(ParkingSpotGroup)));
//     sharedUtils->publicLedger->ledgerVesselNodes = (LedgerVesselNode*)(((uint8_t*)sharedUtils->vesselNodes) + sizeOfVesselNodes);  // sharedUtils->vesselNodesOut = (VesselNode**)sharedUtils + sizeof(SharedUtils) + sizeof(PublicLedger) + 3 * sizeof(ParkingSpotGroup) + sizeOfVesselNodes + sizeOfLedgerVesselNodes;
// }

void getVesselTypeSems(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, sem_t* vesselTypeSem, sem_t* vesselTypeSemPending, char vesselType) {
    for (unsigned int i = 0; i < 3; i++) {
        if (vesselType == parkingSpotGroups[i].type) {
            vesselTypeSem = &sharedUtils->vesselTypesSemIn[i];  /// maybe remove "&" ????????????????? here and below
            vesselTypeSemPending = &sharedUtils->vesselTypesSemPending[i];
            break;
        }
    }
}

void postSemByVesselType(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType) {
    for (unsigned int i = 0; i < 3; i++) {
        if (vesselType == parkingSpotGroups[i].type) {
            sem_post(&sharedUtils->vesselTypesSemIn[i]);  /// maybe remove "&" ????????????????? here and below
            break;
        }
    }
}

void postSemPendingByVesselType(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType) {
    for (unsigned int i = 0; i < 3; i++) {
        if (vesselType == parkingSpotGroups[i].type) {
            sem_post(&sharedUtils->vesselTypesSemPending[i]);  /// maybe remove "&" ????????????????? here and below
            break;
        }
    }
}

void waitSemByVesselType(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType) {
    // printf("in waitSemByVesselType\n");
    for (unsigned int i = 0; i < 3; i++) {
        if (vesselType == parkingSpotGroups[i].type) {
            // doShifts(sharedUtils, sharedUtils->sizeOfVesselNodes, sharedUtils->sizeOfLedgerVesselNodes);  // do necessary shifts
            // printf("vessel is waiting on type semaphore\n");
            sem_wait(&sharedUtils->vesselTypesSemIn[i]);  /// maybe remove "&" ????????????????? here and below
            break;
        }
    }
}

void waitSemPendingByVesselType(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType) {
    // printf("in waitSemPendingByVesselType\n");
    for (unsigned int i = 0; i < 3; i++) {
        if (vesselType == parkingSpotGroups[i].type) {
            // doShifts(sharedUtils, sharedUtils->sizeOfVesselNodes, sharedUtils->sizeOfLedgerVesselNodes);  // do necessary shifts
            // printf("vessel is waiting on type pending semaphore\n");
            sem_wait(&sharedUtils->vesselTypesSemPending[i]);  /// maybe remove "&" ????????????????? here and below
            break;
        }
    }
}

unsigned int getVesselTypeIndex(ParkingSpotGroup* parkingSpotGroups, char vesselType) {
    for (unsigned int i = 0; i < 3; i++) {
        // printf("parking group type: --------------------------------------------------> |%c|\n", parkingSpotGroups[i].type);  //////////////////////// PROBLEM HERE AFTER SOME VESSELS
        if (vesselType == parkingSpotGroups[i].type) {
            // printf("SHIP TYPE GROUP GET type: --------------------------------------------------> |%c|, capacity: %u\n", parkingSpotGroups[i].type, parkingSpotGroups[i].maxCapacity);

            // doShifts(sharedUtils, sharedUtils->sizeOfVesselNodes, sharedUtils->sizeOfLedgerVesselNodes);  // do necessary shifts
            return i;
        }
    }
    printf("Oops.................. get vessel type index failed\n");
    return -1;
}

sem_t* getVesselTypeSem(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType) {
    for (unsigned int i = 0; i < 3; i++) {
        if (vesselType == parkingSpotGroups[i].type) {
            return &sharedUtils->vesselTypesSemIn[i];
        }
    }
    printf("Oops vessel type sem get\n");
    return NULL;
}

ParkingSpotGroup* getVesselParkingSpotGroup(ParkingSpotGroup parkingSpotGroups[3], char vesselType) {
    // printf("SHIP TYPE::::::::::::::::::::::::::::::::::::::::::::::: %c\n", vesselType);
    for (unsigned int i = 0; i < 3; i++) {
        // printf("PARKING SPOT GROUP GET type: --------------------------------------------------> |%c|, capacity: %u\n", parkingSpotGroups[i].type, parkingSpotGroups[i].maxCapacity);
        if (vesselType == parkingSpotGroups[i].type) {
            return &parkingSpotGroups[i];
        }
    }
    printf("Oops parking spot group get\n");
    return NULL;
}

char* vesselStateToString(State state) {
    if (state == WaitingToEnter) {
        return "Waiting to Enter";
    } else if (state == WaitingToLeave) {
        return "Waiting to Leave";
    } else if (state == PendingEnter) {
        return "Pending to Enter";
    } else if (state == Parked) {
        return "Parked";
    } else if (state == Completed) {
        return "Completed";
    } else if (state == Entering) {
        return "Entering";
    } else if (state == Leaving) {
        return "Leaving";
    } else {
        return "N/A";
    }
}

suseconds_t caculateWaitingTime(suseconds_t arrivalTime, suseconds_t departTime, suseconds_t manTime, suseconds_t parkTime) {
    return departTime - arrivalTime - (2 * manTime) - parkTime;
}

void execPortMaster(int shmId, char* logFileName) {
    char shmIdS[MAX_STRING_INT_SIZE];

    sprintf(shmIdS, "%d", shmId);

    char* args[] = {"executables/port-master", "-s", shmIdS, "-lf", logFileName, NULL};
    if (execvp(args[0], args) < 0) {
        printf("Exec port-master failed\n");
    }
}

void execMonitor(long statsPrintTime, int shmId, char* logFileName) {
    char statsPrintTimeS[MAX_STRING_INT_SIZE], shmIdS[MAX_STRING_INT_SIZE];

    sprintf(statsPrintTimeS, "%ld", statsPrintTime);
    sprintf(shmIdS, "%d", shmId);

    char* args[] = {"executables/monitor", "-t", statsPrintTimeS, "-s", shmIdS, "-lf", logFileName, NULL};
    if (execvp(args[0], args) < 0) {
        printf("Exec monitor failed\n");
    }
}

void execVessel(char vesselType, char upgradeType, long parkTime, long manTime, int shmId, char* logFileName) {
    char vesselTypeS[2], upgradeTypeS[2], parkTimeS[MAX_STRING_INT_SIZE], manTimeS[MAX_STRING_INT_SIZE], shmIdS[MAX_STRING_INT_SIZE];
    // printf("fork = 1110\n");

    sprintf(vesselTypeS, "%c", vesselType);
    sprintf(upgradeTypeS, "%c", upgradeType);
    sprintf(parkTimeS, "%ld", parkTime);
    sprintf(manTimeS, "%ld", manTime);
    sprintf(shmIdS, "%d", shmId);

    char* args[] = {"executables/vessel", "-t", vesselTypeS, "-u", upgradeTypeS, "-p", parkTimeS, "-m", manTimeS, "-s", shmIdS, "-lf", logFileName, NULL};
    if (execvp(args[0], args) < 0) {
        printf("Exec vessel failed\n");
    }
}
