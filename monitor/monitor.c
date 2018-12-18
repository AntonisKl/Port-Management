#include "monitor.h"

void handleFlags(int argc, char** argv, suseconds_t* statsPrintTimeUsecs, int* shmId) {
    if (argc != 5) {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    char* ptr;

    if (strcmp(argv[1], "-t") == 0) {
        suseconds_t statsPrintTimeLong = strtol(argv[2], &ptr, 10);
        if (statsPrintTimeLong == 0) {
            printf("Invalid second time argument\nExiting...\n");
            exit(1);
        }

        (*statsPrintTimeUsecs) = statsPrintTimeLong;
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    if (strcmp(argv[3], "-s") == 0) {
        int shmIdInt = atoi(argv[4]);
        if (shmIdInt == 0) {
            printf("Invalid shared memory id argument\nExiting...\n");
            exit(1);
        }

        (*shmId) = shmIdInt;
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }
}

static volatile int keepRunningMonitor = 1;

void intHandler(int sig) {
    keepRunningMonitor = 0;
}

int main(int argc, char** argv) {
    signal(SIGINT, intHandler);

    suseconds_t statsPrintTimeUsecs;
    int shmId;

    handleFlags(argc, argv, &statsPrintTimeUsecs, &shmId);

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

    while (keepRunningMonitor) {
        unsigned long long int vesselsCompleted;
        if (sharedUtils->ledgerSize > 0) {
            printf("\nMonitor's Report:\n\n");
            long double totalIncome = 0;
            unsigned long long int waitingTimesPerType[3], totalWaitingTime = 0, vesselsEntered = 0;
            for (unsigned int i = 0; i < 3; i++)
                waitingTimesPerType[i] = 0;

            vesselsCompleted = 0;

            for (unsigned int i = 0; i < sharedUtils->ledgerSize; i++) {
                printf("Vessel with id %d in the Public Ledger:\nType->%c, Upgrade Type->%c, State->%s, Parking Cost->%f, Arrival Time->%ld ms, Depart Time->%ld ms, Waiting Time->%llu ms, Man Time->%ld ms, Park Time->%ld ms, Parking Spot Type->%c\n",
                       ledgerVesselNodes[i].vesselId, ledgerVesselNodes[i].vesselType,
                       getVesselTypeIndex(parkingSpotGroups, ledgerVesselNodes[i].upgradeType) > getVesselTypeIndex(parkingSpotGroups, ledgerVesselNodes[i].vesselType) ? ledgerVesselNodes[i].upgradeType : '-',
                       vesselStateToString(ledgerVesselNodes[i].state), ledgerVesselNodes[i].stayCost, ledgerVesselNodes[i].arrivalTime,
                       ledgerVesselNodes[i].departTime, ledgerVesselNodes[i].waitingTime, ledgerVesselNodes[i].manTime / 1000, ledgerVesselNodes[i].parkTime / 1000,
                       parkingSpotGroups[ledgerVesselNodes[i].parkingSpotGroupIndex].type);

                totalIncome += (long double)ledgerVesselNodes[i].stayCost;
                vesselsEntered++;
                if (ledgerVesselNodes[i].departTime > 0) {
                    waitingTimesPerType[ledgerVesselNodes[i].parkingSpotGroupIndex] += ledgerVesselNodes[i].waitingTime;
                    totalWaitingTime += ledgerVesselNodes[i].waitingTime;
                    vesselsCompleted++;
                }
            }
            printf("\nStatistics so far:\nTotal vessels in Public Ledger->%u, Total income->%Lf, Average income->%Lf, Total waiting time->%llu ms, Average waiting time->%Lf ms, Waiting time for type %c->%llu ms, Waiting time for type %c->%llu ms, Waiting time for type %c->%llu ms\n\n\n",
                   sharedUtils->ledgerSize, totalIncome, totalIncome / vesselsEntered, totalWaitingTime, vesselsCompleted > 0 ? (long double)totalWaitingTime / vesselsCompleted : 0,
                   parkingSpotGroups[0].type, waitingTimesPerType[0], parkingSpotGroups[1].type, waitingTimesPerType[1], parkingSpotGroups[2].type,
                   waitingTimesPerType[2]);
        }
        if (sharedUtils->ledgerSize > 0 && sharedUtils->ledgerSize == vesselsCompleted) {  // job done, so break the loop and exit
            sharedUtils->monitorDone = 1;
            // wake the port-master up to break his loop
            if (sem_post(&sharedUtils->portMasterWakeSem) < 0) {
                perror("sem_post failed");
                exit(1);
            }
            break;
        }
        usleep(statsPrintTimeUsecs);
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

    return 0;
}