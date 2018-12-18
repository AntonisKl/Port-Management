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

void addLedgerVesselNode(SharedUtils* sharedUtils, VesselNode* vesselNode, LedgerVesselNode* ledgerVesselNodes, ParkingSpotGroup* parkingSpotGroups, char withUpgrade) {
    if (sharedUtils->ledgerSize >= sharedUtils->sizeOfLedgerVesselNodes) {
        printf("Public ledger full\n");
        exit(1);
    }

    unsigned int curVesselTypeIndex;

    // handle upgrade
    if (withUpgrade == 0)
        curVesselTypeIndex = getVesselTypeIndex(parkingSpotGroups, vesselNode->vesselType);
    else
        curVesselTypeIndex = getVesselTypeIndex(parkingSpotGroups, vesselNode->upgradeType);

    unsigned int nextLedgerNodeIndex = sharedUtils->ledgerSize;
    ledgerVesselNodes[nextLedgerNodeIndex].vesselId = vesselNode->vesselId;
    ledgerVesselNodes[nextLedgerNodeIndex].upgradeType = vesselNode->upgradeType;
    ledgerVesselNodes[nextLedgerNodeIndex].vesselType = vesselNode->vesselType;
    ledgerVesselNodes[nextLedgerNodeIndex].arrivalTime = time(NULL);  // current time
    ledgerVesselNodes[nextLedgerNodeIndex].departTime = 0;            // not yet available
    ledgerVesselNodes[nextLedgerNodeIndex].waitingTime = 0;           // not yet available
    ledgerVesselNodes[nextLedgerNodeIndex].parkTime = vesselNode->parkTime;
    ledgerVesselNodes[nextLedgerNodeIndex].manTime = vesselNode->manTime;
    ledgerVesselNodes[nextLedgerNodeIndex].parkingSpotGroupIndex = curVesselTypeIndex;
    ledgerVesselNodes[nextLedgerNodeIndex].stayCost = (((float)vesselNode->parkTime / 1000) / 30) * parkingSpotGroups[curVesselTypeIndex].costPer30Millis;  // handle minutes as milliseconds

    // update the state of vessel's ledger node for the monitor to read it
    if (vesselNode->state == WaitingToEnter || vesselNode->state == PendingEnter)
        ledgerVesselNodes[nextLedgerNodeIndex].state = Entering;
    else if (vesselNode->state == WaitingToLeave)
        ledgerVesselNodes[nextLedgerNodeIndex].state = Leaving;

    vesselNode->ledgerNodeIndex = nextLedgerNodeIndex;  // used to access the corresponding ledger node

    sharedUtils->ledgerSize++;  // update the number of ledger nodes
}

void handleNextVessel(SharedUtils* sharedUtils, VesselNode* vesselNode, LedgerVesselNode* ledgerVesselNodes,
                      ParkingSpotGroup* parkingSpotGroups, unsigned int* numOfPendingVesselNodes,
                      unsigned int* nextIndex, VesselNode** pendingVesselNodeRequests, FILE* logFileP) {
    State curState = vesselNode->state;
    if (curState == WaitingToEnter || curState == PendingEnter) {
        handleIncomingVessel(sharedUtils, vesselNode, parkingSpotGroups, ledgerVesselNodes, nextIndex, pendingVesselNodeRequests, numOfPendingVesselNodes, 0, logFileP);
    } else if (curState == WaitingToLeave) {
        char withUpgrade = vesselNode->withUpgrade;
        // in this point, vesselNode->ledgerNodeIndex should be >0 but sometimes it isn't because the FIFO order is not always kept on the semaphores waiting
        if (vesselNode->ledgerNodeIndex == -1) {  // for by-passing POSIX non-FIFO semaphores problem
            addLedgerVesselNode(sharedUtils, vesselNode, ledgerVesselNodes, parkingSpotGroups, withUpgrade);
        }

        fprintf(logFileP, "Timestamp (millis): %lu -> Port master starts handling outgoing vessel with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
        handleOutGoingVessel(sharedUtils, vesselNode, parkingSpotGroups, ledgerVesselNodes, nextIndex, withUpgrade);
        fprintf(logFileP, "Timestamp (millis): %lu -> Port master finished handling outgoing vessel with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
    } else if (curState == Completed) {
        (*nextIndex)++;
    }
}

void handleOutGoingVessel(SharedUtils* sharedUtils, VesselNode* vesselNode, ParkingSpotGroup* parkingSpotGroups, LedgerVesselNode* ledgerVesselNodes, unsigned int* nextIndex, char withUpgrade) {
    char curVesselType = vesselNode->vesselType;
    unsigned int curVesselTypeIndex;
    if (withUpgrade == 0)
        curVesselTypeIndex = getVesselTypeIndex(parkingSpotGroups, curVesselType);
    else
        curVesselTypeIndex = getVesselTypeIndex(parkingSpotGroups, vesselNode->upgradeType);

    sem_wait(&sharedUtils->inOutSem);

    postSemByVesselType(sharedUtils, parkingSpotGroups, curVesselType);

    ledgerVesselNodes[vesselNode->ledgerNodeIndex].state = Leaving;

    usleep(vesselNode->manTime);

    // update vessel's public ledger node
    ledgerVesselNodes[vesselNode->ledgerNodeIndex].state = Completed;
    ledgerVesselNodes[vesselNode->ledgerNodeIndex].departTime = time(NULL);
    ledgerVesselNodes[vesselNode->ledgerNodeIndex].waitingTime = vesselNode->waitingTime;

    // free a spot
    parkingSpotGroups[curVesselTypeIndex].curCapacity++;

    (*nextIndex)++;
}

void handleIncomingVessel(SharedUtils* sharedUtils, VesselNode* vesselNode, ParkingSpotGroup* parkingSpotGroups,
                          LedgerVesselNode* ledgerVesselNodes, unsigned int* nextIndex,
                          VesselNode* pendingVesselNodeRequests[100], unsigned int* numOfPendingVesselNodes, char withUpgrade, FILE* logFileP) {
    char curVesselType = vesselNode->vesselType;
    unsigned int curVesselTypeIndex;
    if (withUpgrade == 0)
        curVesselTypeIndex = getVesselTypeIndex(parkingSpotGroups, curVesselType);
    else if (withUpgrade == 1)
        curVesselTypeIndex = getVesselTypeIndex(parkingSpotGroups, vesselNode->upgradeType);

    vesselNode->withUpgrade = withUpgrade;

    State curState = vesselNode->state;
    // check for available place
    if (parkingSpotGroups[curVesselTypeIndex].curCapacity == 0 && curState != PendingEnter) {  // no capcity and vessel is waiting in the normal queue
        if (withUpgrade == 0) {
            fprintf(logFileP, "Timestamp (millis): %lu -> Port master starts handling incoming vessel with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
        } else if (withUpgrade == 1) {
            fprintf(logFileP, "Timestamp (millis): %lu -> Port master starts handling incoming vessel with upgrade with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
        }

        // if there is ability for upgrade, call this function recursively with withUpgrade = 1
        if (withUpgrade == 0 && (getVesselTypeIndex(parkingSpotGroups, vesselNode->upgradeType) > getVesselTypeIndex(parkingSpotGroups, curVesselType))) {
            handleIncomingVessel(sharedUtils, vesselNode, parkingSpotGroups, ledgerVesselNodes, nextIndex, pendingVesselNodeRequests,
                                 numOfPendingVesselNodes, 1, logFileP);
            return;
        }

        // add vessel to the pending queue
        vesselNode->state = PendingEnter;

        pendingVesselNodeRequests[*numOfPendingVesselNodes] = vesselNode;
        (*numOfPendingVesselNodes)++;

        // allow to the vessel to proceed in waiting on the pending semaphore
        postSemByVesselType(sharedUtils, parkingSpotGroups, curVesselType);
        (*nextIndex)++;

        // log
        if (withUpgrade == 0)
            fprintf(logFileP, "Timestamp (millis): %lu -> Port master finished handling incoming vessel with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
        else if (withUpgrade == 1)
            fprintf(logFileP, "Timestamp (millis): %lu -> Port master finished handling incoming vessel with upgrade with pid %d\n", (unsigned long)time(NULL), vesselNode->vesselId);
    } else if (parkingSpotGroups[curVesselTypeIndex].curCapacity > 0) {
        // log
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

        // post normal queue's semaphore or pending queue's semaphore to let the vessel get in the port
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
        // we can do nothing with the current vessel yet
        usleep(500000);  // sleep for 0.5 seconds to leave time for places to be freed and then continue port-master's main loop
    }
}

static volatile int keepRunningPortMaster = 1;

// handler for SIGINT
void intHandler(int sig) {
    keepRunningPortMaster = 0;
}

int main(int argc, char** argv) {
    // start signal handler
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

    VesselNode* pendingVesselNodeRequests[sharedUtils->sizeOfVesselNodes / sizeof(VesselNode)];  // this is the queue where the vessels that cannot yet enter the port will be sent to
    unsigned int nextIndex = 0, nextPendingIndex = 0, numOfPendingVesselNodes = 0;               // nextIndex: the index of the normal queue of the next vessel to be handled
                                                                                                 // nextPendingIndex: the index of the pending queue of the next vessel to be handled
                                                                                                 // numOfPendingVesselNodes: number of vessels currently in the pending queue
    FILE* logFileP;
    if ((logFileP = fopen(logFileName, "a")) == NULL) {
        perror("fopen failed");
    }

    char didSomething = 1;
    while (keepRunningPortMaster) {
        if (didSomething == 1) {
            // log
            fprintf(logFileP, "Timestamp (millis): %lu -> Port master is waiting for vessels\n", (unsigned long)time(NULL));
            fflush(logFileP);
            didSomething = 0;
        }
        // wait for vessel to wake port-master up
        sem_wait(&sharedUtils->portMasterWakeSem);

        if (sharedUtils->monitorDone == 1) {  // job done so break the loop and exit
            break;
        }

        while (keepRunningPortMaster && (nextIndex < sharedUtils->queueSize || nextPendingIndex < numOfPendingVesselNodes)) {
            unsigned int prevNextIndex = nextIndex, prevNextPendingIndex = nextPendingIndex;  // used for not logging duplicate strings

            if (nextPendingIndex < numOfPendingVesselNodes) {
                handleNextVessel(sharedUtils, pendingVesselNodeRequests[nextPendingIndex], ledgerVesselNodes, parkingSpotGroups, NULL, &nextPendingIndex, NULL, logFileP);
            }

            if (nextIndex < sharedUtils->queueSize) {
                handleNextVessel(sharedUtils, &vesselNodes[nextIndex], ledgerVesselNodes, parkingSpotGroups,
                                 &numOfPendingVesselNodes, &nextIndex, pendingVesselNodeRequests, logFileP);
            }

            if (prevNextIndex != nextIndex || prevNextPendingIndex != nextPendingIndex) {
                didSomething = 1;
                if (fflush(logFileP) == EOF) {
                    perror("fflush failed");
                    exit(1);
                }
            }
        }
    }

    if (fclose(logFileP) == EOF) {
        perror("fclose failed");
    }

    // destroy semaphores as they are not needed anymore
    sem_destroy(&sharedUtils->inOutSem);
    sem_destroy(&sharedUtils->shmWriteSem);
    for (unsigned int i = 0; i < 3; i++) {
        sem_destroy(&sharedUtils->vesselTypesSem[i]);
        sem_destroy(&sharedUtils->vesselTypesSemPending[i]);
    }

    // dettach shared memory segments
    if (shmdt(sharedUtils) == -1) {
        perror("shmdt failed");
        exit(1);
    }
    if (shmdt(vesselNodes) == -1) {
        perror("shmdt failed");
        exit(1);
    }
    if (shmdt(ledgerVesselNodes) == -1) {
        perror("shmdt failed");
        exit(1);
    }
    if (shmdt(parkingSpotGroups) == -1) {
        perror("shmdt failed");
        exit(1);
    }
}
