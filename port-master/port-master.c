#include "port-master.h"

void handleFlags(int argc, char** argv, int* shmId, char** logFileName) {
    if (argc != 5) {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

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

State getNextPendingVesselState(State state) {
    if (state == WaitingToEnter)
        return PendingEnter;
    else
        return state;  // this will not happen
}

void addLedgerVesselNode(SharedUtils* sharedUtils, VesselNode* vesselNode, LedgerVesselNode* ledgerVesselNodes, ParkingSpotGroup* parkingSpotGroups, char withUpgrade) {
    if (sharedUtils->ledgerSize >= sharedUtils->sizeOfLedgerVesselNodes) {
        printf("Public ledger full\n");
        exit(1);
    }

    struct timeval curTime;
    unsigned int curVesselTypeIndex;
    if (withUpgrade == 0)
        curVesselTypeIndex = getVesselTypeIndex(parkingSpotGroups, vesselNode->vesselType);
    else
        curVesselTypeIndex = getVesselTypeIndex(parkingSpotGroups, vesselNode->upgradeType);

    gettimeofday(&curTime, NULL);

    unsigned int nextLedgerNodeIndex = sharedUtils->ledgerSize;
    ledgerVesselNodes[nextLedgerNodeIndex].vesselId = vesselNode->vesselId;
    ledgerVesselNodes[nextLedgerNodeIndex].upgradeType = vesselNode->upgradeType;
    ledgerVesselNodes[nextLedgerNodeIndex].vesselType = vesselNode->vesselType;
    ledgerVesselNodes[nextLedgerNodeIndex].arrivalTime = time(NULL);
    ledgerVesselNodes[nextLedgerNodeIndex].departTime = -1;
    ledgerVesselNodes[nextLedgerNodeIndex].waitingTime = 0;
    ledgerVesselNodes[nextLedgerNodeIndex].parkTime = vesselNode->parkTime;
    ledgerVesselNodes[nextLedgerNodeIndex].manTime = vesselNode->manTime;
    ledgerVesselNodes[nextLedgerNodeIndex].parkingSpotGroupIndex = curVesselTypeIndex;
    // improve cost mechanism
    ledgerVesselNodes[nextLedgerNodeIndex].stayCost = (((float)vesselNode->parkTime / 1000) / 30) * parkingSpotGroups[curVesselTypeIndex].costPer30Millis;  // handle minutes as milliseconds
    if (vesselNode->state == WaitingToEnter || vesselNode->state == PendingEnter)
        ledgerVesselNodes[nextLedgerNodeIndex].state = Entering;
    else if (vesselNode->state == WaitingToLeave)
        ledgerVesselNodes[nextLedgerNodeIndex].state = Leaving;

    vesselNode->ledgerNodeIndex = nextLedgerNodeIndex;  /// point to ledger node

    sharedUtils->ledgerSize++;
}

void handleNextVessel(SharedUtils* sharedUtils, VesselNode* vesselNode, LedgerVesselNode* ledgerVesselNodes,
                      ParkingSpotGroup* parkingSpotGroups, unsigned int* sizeOfPendingVesselNodes,
                      unsigned int* nextIndex, VesselNode* pendingVesselNodeRequests[100], FILE* logFileP) {
    State curState = vesselNode->state;
    if (curState == WaitingToEnter || curState == PendingEnter) {
        handleIncomingVessel(sharedUtils, vesselNode, parkingSpotGroups, ledgerVesselNodes, nextIndex, pendingVesselNodeRequests, sizeOfPendingVesselNodes, 0, logFileP);
    } else if (curState == WaitingToLeave) {
        char withUpgrade = vesselNode->withUpgrade;
        if (vesselNode->ledgerNodeIndex == -1) {  // for by-passing POSIX non-FIFO semaphores
            addLedgerVesselNode(sharedUtils, vesselNode, ledgerVesselNodes, parkingSpotGroups, withUpgrade);
        }

        fprintf(logFileP, "Timestamp (millis): %lu -> Port master starts handling outgoing vessel with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
        handleOutGoingVessel(sharedUtils, vesselNode, parkingSpotGroups, ledgerVesselNodes, nextIndex, withUpgrade);
        fprintf(logFileP, "Timestamp (millis): %lu -> Port master finished handling outgoing vessel with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
    }
}

void handleOutGoingVessel(SharedUtils* sharedUtils, VesselNode* vesselNode, ParkingSpotGroup* parkingSpotGroups, LedgerVesselNode* ledgerVesselNodes, unsigned int* nextIndex, char withUpgrade) {
    char curVesselType = vesselNode->vesselType;
    unsigned int curVesselTypeIndex;
    if (withUpgrade == 0)
        curVesselTypeIndex = getVesselTypeIndex(parkingSpotGroups, curVesselType);
    else
        curVesselTypeIndex = getVesselTypeIndex(parkingSpotGroups, vesselNode->upgradeType);

    State curState = vesselNode->state;

    sem_wait(&sharedUtils->inOutSem);

    if (curState == WaitingToLeave)
        postSemByVesselType(sharedUtils, parkingSpotGroups, curVesselType);

    usleep(vesselNode->manTime);

    ledgerVesselNodes[vesselNode->ledgerNodeIndex].state = Completed;
    ledgerVesselNodes[vesselNode->ledgerNodeIndex].departTime = time(NULL);
    ledgerVesselNodes[vesselNode->ledgerNodeIndex].waitingTime = vesselNode->waitingTime;

    parkingSpotGroups[curVesselTypeIndex].curCapacity++;

    (*nextIndex)++;
}

void handleIncomingVessel(SharedUtils* sharedUtils, VesselNode* vesselNode, ParkingSpotGroup* parkingSpotGroups,
                          LedgerVesselNode* ledgerVesselNodes, unsigned int* nextIndex,
                          VesselNode* pendingVesselNodeRequests[100], unsigned int* sizeOfPendingVesselNodes, char withUpgrade, FILE* logFileP) {
    char curVesselType = vesselNode->vesselType;
    unsigned int curVesselTypeIndex;
    if (withUpgrade == 0)
        curVesselTypeIndex = getVesselTypeIndex(parkingSpotGroups, curVesselType);
    else if (withUpgrade == 1)
        curVesselTypeIndex = getVesselTypeIndex(parkingSpotGroups, vesselNode->upgradeType);

    vesselNode->withUpgrade = withUpgrade;

    State curState = vesselNode->state;

    // check for available place
    if (parkingSpotGroups[curVesselTypeIndex].curCapacity == 0 && curState != PendingEnter) {
        if (withUpgrade == 0) {
            fprintf(logFileP, "Timestamp (millis): %lu -> Port master starts handling incoming vessel with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
        } else if (withUpgrade == 1)
            fprintf(logFileP, "Timestamp (millis): %lu -> Port master starts handling incoming vessel with upgrade with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);

        if (withUpgrade == 0 && (getVesselTypeIndex(parkingSpotGroups, vesselNode->upgradeType) > getVesselTypeIndex(parkingSpotGroups, curVesselType))) {
            handleIncomingVessel(sharedUtils, vesselNode, parkingSpotGroups, ledgerVesselNodes, nextIndex, pendingVesselNodeRequests,
                                 sizeOfPendingVesselNodes, 1, logFileP);
            return;
        }

        vesselNode->state = getNextPendingVesselState(curState);

        pendingVesselNodeRequests[*sizeOfPendingVesselNodes] = vesselNode;
        (*sizeOfPendingVesselNodes)++;

        postSemByVesselType(sharedUtils, parkingSpotGroups, curVesselType);
        (*nextIndex)++;
        if (withUpgrade == 0)
            fprintf(logFileP, "Timestamp (millis): %lu -> Port master finished handling incoming vessel with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
        else if (withUpgrade == 1)
            fprintf(logFileP, "Timestamp (millis): %lu -> Port master finished handling incoming vessel with upgrade with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
    } else if (parkingSpotGroups[curVesselTypeIndex].curCapacity > 0) {
        if (curState == WaitingToEnter) {
            if (withUpgrade == 0)
                fprintf(logFileP, "Timestamp (millis): %lu -> Port master starts handling incoming vessel with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
            else if (withUpgrade == 1)
                fprintf(logFileP, "Timestamp (millis): %lu -> Port master starts handling incoming vessel with upgrade with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
        } else if (curState == PendingEnter) {
            if (withUpgrade == 0)
                fprintf(logFileP, "Timestamp (millis): %lu -> Port master starts handling pending incoming vessel with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
            else if (withUpgrade == 1)
                fprintf(logFileP, "Timestamp (millis): %lu -> Port master starts handling pending incoming vessel with upgrade with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
        }
        parkingSpotGroups[curVesselTypeIndex].curCapacity--;
        addLedgerVesselNode(sharedUtils, vesselNode, ledgerVesselNodes, parkingSpotGroups, withUpgrade);

        sem_wait(&sharedUtils->inOutSem);
        if (curState == WaitingToEnter) {
            postSemByVesselType(sharedUtils, parkingSpotGroups, curVesselType);
            if (withUpgrade == 0)
                fprintf(logFileP, "Timestamp (millis): %lu -> Port master finished handling incoming vessel with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
            else if (withUpgrade == 1)
                fprintf(logFileP, "Timestamp (millis): %lu -> Port master finished handling incoming vessel with upgrade with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
        } else if (curState == PendingEnter) {
            postSemPendingByVesselType(sharedUtils, parkingSpotGroups, curVesselType);
            if (withUpgrade == 0)
                fprintf(logFileP, "Timestamp (millis): %lu -> Port master finished handling pending incoming vessel with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
            else if (withUpgrade == 1)
                fprintf(logFileP, "Timestamp (millis): %lu -> Port master finished handling pending incoming vessel with upgrade with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
        }
        (*nextIndex)++;
    } else if (parkingSpotGroups[curVesselTypeIndex].curCapacity == 0 && curState == PendingEnter) {
        usleep(500000);  // sleep for 0.5 seconds to leave time for places to be freed
    }
}

static volatile int keepRunningPortMaster = 1;

void intHandler(int sig) {
    keepRunningPortMaster = 0;
}

int main(int argc, char** argv) {
    signal(SIGINT, intHandler);

    int shmId;
    char* logFileName;
    handleFlags(argc, argv, &shmId, &logFileName);

    SharedUtils* sharedUtils = (SharedUtils*)((uint8_t*)shmat(shmId, 0, 0));
    if (sharedUtils == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }

    ParkingSpotGroup* parkingSpotGroups = (ParkingSpotGroup*)((uint8_t*)shmat(sharedUtils->shmIdParkingSpotGroups, 0, 0));
    if (parkingSpotGroups == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }
    VesselNode* vesselNodes = (VesselNode*)((uint8_t*)shmat(sharedUtils->shmIdVesselNodes, 0, 0));
    if (vesselNodes == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }
    LedgerVesselNode* ledgerVesselNodes = (LedgerVesselNode*)((uint8_t*)shmat(sharedUtils->shmIdLedgerNodes, 0, 0));
    if (ledgerVesselNodes == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }

    unsigned int queueSize = 0;

    VesselNode* pendingVesselNodeRequests[sharedUtils->sizeOfVesselNodes / sizeof(VesselNode)];
    unsigned int nextInIndex = 0, nextPendingIndex = 0, sizeOfPendingVesselNodes = 0;
    FILE* logFileP;
    if ((logFileP = fopen(logFileName, "a")) == NULL) {
        perror("fopen failed");
    }

    char didSomething = 1;
    while (keepRunningPortMaster) {
        if (didSomething == 1) {
            fprintf(logFileP, "Timestamp (millis): %lu -> Port master is waiting for vessels\n", (unsigned long)time(NULL));
            fflush(logFileP);
            didSomething = 0;
        }
        sem_wait(&sharedUtils->portMasterWakeSem);

        queueSize = sharedUtils->queueSize;

        while (keepRunningPortMaster && (nextInIndex < queueSize || nextPendingIndex < sizeOfPendingVesselNodes)) {
            unsigned int prevNextIndex = nextInIndex, prevNextPendingIndex = nextPendingIndex;

            if (nextPendingIndex < sizeOfPendingVesselNodes) {
                handleNextVessel(sharedUtils, pendingVesselNodeRequests[nextPendingIndex], ledgerVesselNodes, parkingSpotGroups, NULL, &nextPendingIndex, NULL, logFileP);
            }

            queueSize = sharedUtils->queueSize;
            if (nextInIndex < queueSize) {
                State curState = vesselNodes[nextInIndex].state;
                if (curState == WaitingToEnter || curState == WaitingToLeave) {
                    handleNextVessel(sharedUtils, &vesselNodes[nextInIndex], ledgerVesselNodes, parkingSpotGroups,
                                     &sizeOfPendingVesselNodes, &nextInIndex, pendingVesselNodeRequests, logFileP);
                }
            }

            if (prevNextIndex != nextInIndex || prevNextPendingIndex != nextPendingIndex) {
                didSomething = 1;
            }
            fflush(logFileP);
        }
    }

    if (fclose(logFileP) == EOF) {
        perror("fclose failed");
    }

    sem_destroy(&sharedUtils->inOutSem);
    sem_destroy(&sharedUtils->shmWriteSem);
    for (unsigned int i = 0; i < 3; i++) {
        sem_destroy(&sharedUtils->vesselTypesSemIn[i]);
        sem_destroy(&sharedUtils->vesselTypesSemPending[i]);
    }

    if (shmdt(sharedUtils) == -1) {
        perror("shmat failed");
        exit(1);
    }
    if (shmdt(vesselNodes) == -1) {
        perror("shmat failed");
        exit(1);
    }

    if (shmdt(ledgerVesselNodes) == -1) {
        perror("shmat failed");
        exit(1);
    }
    if (shmdt(parkingSpotGroups) == -1) {
        perror("shmat failed");
        exit(1);
    }
}
