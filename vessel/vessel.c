#include "vessel.h"

void handleFlags(int argc, char** argv, char* type, char* upgradeType, suseconds_t* parkTime, suseconds_t* manTime, int* shmId, char** logFileName) {
    if (argc != 13) {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    char* ptr;

    if (strcmp(argv[1], "-t") == 0) {
        (*type) = argv[2][0];
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    if (strcmp(argv[3], "-u") == 0) {
        (*upgradeType) = argv[4][0];
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    if (strcmp(argv[5], "-p") == 0) {
        long parkTimeLong = strtol(argv[6], &ptr, 10);
        if (parkTimeLong == 0) {
            printf("Invalid park time argument\nExiting...\n");
            exit(1);
        }

        (*parkTime) = parkTimeLong * 1000;  // convert to microseconds
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    if (strcmp(argv[7], "-m") == 0) {
        long manTimeLong = strtol(argv[8], &ptr, 10);
        if (manTimeLong == 0) {
            printf("Invalid park time argument\nExiting...\n");
            exit(1);
        }

        (*manTime) = manTimeLong * 1000;  // convert to microseconds
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    if (strcmp(argv[9], "-s") == 0) {
        int shmIdInt = atoi(argv[10]);
        if (shmIdInt == 0) {
            printf("Invalid shared memory id argument\nExiting...\n");
            exit(1);
        }

        (*shmId) = shmIdInt;
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    if (strcmp(argv[11], "-lf") == 0) {
        (*logFileName) = argv[12];
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }
}

VesselNode* addVesselNodeToShmByVesselNode(SharedUtils* sharedUtils, VesselNode* vesselNodes, VesselNode vesselNode) {
    sem_wait(&sharedUtils->shmWriteSem);  // start of critical section

    unsigned int nextVesselNodeIndex = sharedUtils->queueSize;

    vesselNodes[nextVesselNodeIndex].vesselType = vesselNode.vesselType;
    vesselNodes[nextVesselNodeIndex].parkTime = vesselNode.parkTime;
    vesselNodes[nextVesselNodeIndex].vesselId = getpid();
    vesselNodes[nextVesselNodeIndex].manTime = vesselNode.manTime;
    vesselNodes[nextVesselNodeIndex].upgradeType = vesselNode.upgradeType;
    vesselNodes[nextVesselNodeIndex].state = vesselNode.state;
    vesselNodes[nextVesselNodeIndex].withUpgrade = vesselNode.withUpgrade;
    vesselNodes[nextVesselNodeIndex].waitingTime = vesselNode.waitingTime;
    vesselNodes[nextVesselNodeIndex].ledgerNodeIndex = vesselNode.ledgerNodeIndex;

    // update queue's size
    sharedUtils->queueSize++;

    sem_post(&sharedUtils->shmWriteSem);  // end of critical section

    return &vesselNodes[nextVesselNodeIndex];
}

VesselNode* addVesselNodeToShmByValues(SharedUtils* sharedUtils, VesselNode* vesselNodes, char vesselType, char upgradeType, suseconds_t parkTime, suseconds_t manTime, State state) {
    sem_wait(&sharedUtils->shmWriteSem);
    unsigned int nextVesselNodeIndex = sharedUtils->queueSize;
    vesselNodes[nextVesselNodeIndex].vesselType = vesselType;
    vesselNodes[nextVesselNodeIndex].parkTime = parkTime;
    vesselNodes[nextVesselNodeIndex].vesselId = getpid();
    vesselNodes[nextVesselNodeIndex].manTime = manTime;
    vesselNodes[nextVesselNodeIndex].upgradeType = upgradeType;
    vesselNodes[nextVesselNodeIndex].state = state;
    vesselNodes[nextVesselNodeIndex].withUpgrade = 0;
    vesselNodes[nextVesselNodeIndex].waitingTime = 0;

    // first time adding vessel to queue so there is no corresponding ledger node
    vesselNodes[nextVesselNodeIndex].ledgerNodeIndex = -1;

    sharedUtils->queueSize++;

    if (sem_post(&sharedUtils->shmWriteSem) < 0)
        perror("sem post failed");

    return &vesselNodes[nextVesselNodeIndex];  // return a ponter that points to the added vessel node
}

int main(int argc, char** argv) {
    char vesselType, upgradeType, *logFileName;
    int shmId;
    suseconds_t parkTimeUsecs, manTimeUsecs;

    handleFlags(argc, argv, &vesselType, &upgradeType, &parkTimeUsecs, &manTimeUsecs, &shmId, &logFileName);

    // attach shared memory segments
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

    long long unsigned int waitingTime = 0;
    long timeStart, timeStop;  // used for calculating waiting time

    FILE* logFileP;
    if ((logFileP = fopen(logFileName, "a")) == NULL) {
        perror("fopen failed");
    }

    VesselNode* vesselNode = addVesselNodeToShmByValues(sharedUtils, vesselNodes, vesselType, upgradeType, parkTimeUsecs, manTimeUsecs, WaitingToEnter);

    if (sem_post(&sharedUtils->portMasterWakeSem) < 0) {
        perror("portMasterWakeSem failed");
        exit(1);
    }

    fprintf(logFileP, "Timestamp (millis): %lu -> Vessel with pid %d and type %c entered the queue as incoming\n", (unsigned long)time(NULL), vesselNode->vesselId, vesselNode->vesselType);
    timeStart = time(NULL);
    waitSemByVesselType(sharedUtils, parkingSpotGroups, vesselType);  // wait for port-master to allow the vessel to get into the port OR to wait on the pending semaphore

    if (vesselNode->state == PendingEnter) {
        fprintf(logFileP, "Timestamp (millis): %lu -> Vessel with pid %d and type %c entered the pending queue\n", (unsigned long)time(NULL), vesselNode->vesselId, vesselNode->vesselType);
        waitSemPendingByVesselType(sharedUtils, parkingSpotGroups, vesselType);  // port-master ordered the vessel to wait further on the pending semaphore
    }
    timeStop = time(NULL);

    waitingTime += ((unsigned long long int)timeStop - timeStart);
    vesselNode->waitingTime = waitingTime;

    fprintf(logFileP, "Timestamp (millis): %lu -> Vessel with pid %d and type %c entered the port\n", (unsigned long)time(NULL), vesselNode->vesselId, vesselNode->vesselType);

    usleep(manTimeUsecs);  // sleep for manTime + parkTime

    if (sem_post(&sharedUtils->inOutSem) < 0) {  // allow the port-master to get another vessel in/out of the port
        perror("sem_post failed");
        exit(1);
    }

    vesselNode->state = Parked;

    fprintf(logFileP, "Timestamp (millis): %lu -> Vessel with pid %d and type %c parked in the port\n", (unsigned long)time(NULL), vesselNode->vesselId, vesselNode->vesselType);

    usleep(parkTimeUsecs);

    vesselNode->state = WaitingToLeave;
    vesselNode = addVesselNodeToShmByVesselNode(sharedUtils, vesselNodes, *vesselNode);  // add the vessel node again but with updated values

    if (sem_post(&sharedUtils->portMasterWakeSem) < 0)
        perror("portMasterWakeSem failed");

    timeStart = time(NULL);
    waitSemByVesselType(sharedUtils, parkingSpotGroups, vesselNode->vesselType);
    timeStop = time(NULL);

    waitingTime += ((unsigned long long int)timeStop - timeStart);
    vesselNode->waitingTime = waitingTime;

    usleep(manTimeUsecs);

    vesselNode->state = Completed;
    if (sem_post(&sharedUtils->inOutSem) < 0) {  // allow the port-master to get another vessel in/out of the port
        perror("sem_post failed");
        exit(1);
    }
    fprintf(logFileP, "Timestamp (millis): %lu -> Vessel with pid %d and type %c left the port\n", (unsigned long)time(NULL), vesselNode->vesselId, vesselNode->vesselType);

    // close log file
    if (fclose(logFileP) == EOF) {
        perror("fclose failed");
    }

    // detach shared memory segments
    if (shmdt(sharedUtils) == -1) {
        perror("shmdt failed");
        exit(1);
    }
    if (shmdt(vesselNodes) == -1) {
        perror("shmdt failed");
        exit(1);
    }
    if (shmdt(parkingSpotGroups) == -1) {
        perror("shmdt failed");
        exit(1);
    }

    return 0;
}
