#include "utils.h"

void initParkingSpotGroup(ParkingSpotGroup* parkingSpot, char type, unsigned int maxCapacity, float cost) {
    parkingSpot->maxCapacity = maxCapacity;
    parkingSpot->costPer30Millis = cost;
    parkingSpot->type = type;
    parkingSpot->curCapacity = maxCapacity;
}

void initSharedUtilsAndParkingSpotGroups(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char* parkingSpotTypes,
                                         unsigned int* capacities, float* costs, sem_t inOutSem, sem_t vesselTypesSem[3], sem_t vesselTypesSemOut[3], int sizeOfVesselNodes,
                                         int sizeOfLedgerVesselNodes, sem_t shmWriteSem, sem_t portMasterWakeSem, int shmIdParkingGroups, int shmIdVesselNodes, int shmIdLedgerNodes) {
    sharedUtils->inOutSem = inOutSem;
    sharedUtils->shmWriteSem = shmWriteSem;
    sharedUtils->sizeOfVesselNodes = sizeOfVesselNodes;
    sharedUtils->sizeOfLedgerVesselNodes = sizeOfLedgerVesselNodes;

    sharedUtils->vesselTypesSem[0] = vesselTypesSem[0];
    sharedUtils->vesselTypesSem[1] = vesselTypesSem[1];
    sharedUtils->vesselTypesSem[2] = vesselTypesSem[2];

    sharedUtils->vesselTypesSemPending[0] = vesselTypesSemOut[0];
    sharedUtils->vesselTypesSemPending[1] = vesselTypesSemOut[1];
    sharedUtils->vesselTypesSemPending[2] = vesselTypesSemOut[2];

    sharedUtils->shmIdParkingSpotGroups = shmIdParkingGroups;
    sharedUtils->shmIdVesselNodes = shmIdVesselNodes;
    sharedUtils->shmIdLedgerNodes = shmIdLedgerNodes;

    sharedUtils->portMasterWakeSem = portMasterWakeSem;

    sharedUtils->queueSize = 0;
    sharedUtils->ledgerSize = 0;

    for (unsigned int i = 0; i < 3; i++) {
        initParkingSpotGroup(&(parkingSpotGroups[i]), parkingSpotTypes[i], capacities[i], costs[i]);
    }
}

void postSemByVesselType(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType) {
    for (unsigned int i = 0; i < 3; i++) {
        if (vesselType == parkingSpotGroups[i].type) {
            if (sem_post(&sharedUtils->vesselTypesSem[i]) < 0) {
                perror("sem_post failed");
                exit(1);
            }
            break;
        }
    }
}

void postSemPendingByVesselType(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType) {
    for (unsigned int i = 0; i < 3; i++) {
        if (vesselType == parkingSpotGroups[i].type) {
            if (sem_post(&sharedUtils->vesselTypesSemPending[i]) < 0) {
                perror("sem_post failed");
                exit(1);
            }
            break;
        }
    }
}

void waitSemByVesselType(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType) {
    for (unsigned int i = 0; i < 3; i++) {
        if (vesselType == parkingSpotGroups[i].type) {
            if (sem_wait(&sharedUtils->vesselTypesSem[i]) < 0) {
                perror("sem_wait failed");
                exit(1);
            }
            break;
        }
    }
}

void waitSemPendingByVesselType(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType) {
    for (unsigned int i = 0; i < 3; i++) {
        if (vesselType == parkingSpotGroups[i].type) {
            if (sem_wait(&sharedUtils->vesselTypesSemPending[i]) < 0) {
                perror("sem_wait failed");
                exit(1);
            }
            break;
        }
    }
}

unsigned int getVesselTypeIndex(ParkingSpotGroup* parkingSpotGroups, char vesselType) {
    for (unsigned int i = 0; i < 3; i++) {
        if (vesselType == parkingSpotGroups[i].type) {
            return i;
        }
    }
    printf("Oops... get vessel type index failed\n");
    exit(1);
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
