#include "myport.h"

#define MAX_PARK_TIME 500 // max park time value for random generation
#define MAX_MAN_TIME 500 // max man time value for random generation

void handleFlags(int argc, char** argv, char** configFileName, unsigned int* vesselsNum, suseconds_t* vesselIntervalUsecs, suseconds_t* monitorIntervalUsecs) {
    if (argc != 3 && argc != 9) {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    if (strcmp(argv[1], "-l") == 0) {
        (*configFileName) = argv[2];
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    if (argc == 9) {
        if (strcmp(argv[3], "-n") == 0) {
            (*vesselsNum) = atoi(argv[4]);
            if ((*vesselsNum) == 0) {
                printf("Invalid flags\nExiting...\n");
                exit(1);
            }
        } else {
            printf("Invalid flags\nExiting...\n");
            exit(1);
        }

        if (strcmp(argv[5], "-i") == 0) {
            (*vesselIntervalUsecs) = atol(argv[6]) * 1000;
            if ((*vesselIntervalUsecs) < 0) {
                printf("Invalid flags\nExiting...\n");
                exit(1);
            }
        } else {
            printf("Invalid flags\nExiting...\n");
            exit(1);
        }

        if (strcmp(argv[7], "-m") == 0) {
            (*monitorIntervalUsecs) = atol(argv[8]) * 1000;
            if ((*monitorIntervalUsecs) < 0) {
                printf("Invalid flags\nExiting...\n");
                exit(1);
            }
        } else {
            printf("Invalid flags\nExiting...\n");
            exit(1);
        }
    }
}

void execRandomVessels(unsigned int vesselsNum, suseconds_t intervalUsecs, char* vesselTypes, int shmIdSharedUtils, char* logFileName) {
    srand(time(NULL));

    for (unsigned int i = 0; i < vesselsNum; i++) {
        unsigned int typeIndex = rand() % 3;
        unsigned int upgradeIndex = rand() % 3;  // if upgradeIndex <= typeIndex then later the vessel is not considered upgradeable by the port-master
        long parkTimeMillis = (rand() % MAX_PARK_TIME) + 1;
        long manTimePeriodMillis = (rand() % MAX_MAN_TIME) + 1;

        if (fork() == 0) {
            execVessel(vesselTypes[typeIndex], vesselTypes[upgradeIndex], parkTimeMillis, manTimePeriodMillis, shmIdSharedUtils, logFileName);
        }
        usleep(intervalUsecs);
    }
}

int main(int argc, char** argv) {
    char* configFileName;
    char parkingSpotTypes[3];
    float costsPer30Millis[3];
    unsigned int parkingSpotCapacities[3], vesselsNum = -1;
    suseconds_t spawnIntervalUsecs = -1, monitorIntervalUsecs = -1;

    handleFlags(argc, argv, &configFileName, &vesselsNum, &spawnIntervalUsecs, &monitorIntervalUsecs);

    FILE *configFileP, *logFileP;
    char line[30];

    if ((configFileP = fopen(configFileName, "r")) == NULL) {
        perror("fopen config file");
        return 1;
    }

    unsigned int index = 0;

    // read from configuration file
    while (fgets(line, sizeof(line), configFileP) != NULL && index < 9) {
        line[strlen(line) - 1] = '\0';  // cut newline character
        printf("%s\n", line);
        if (index >= 0 && index < 3) {  // first 3 lines
            parkingSpotTypes[index] = line[strlen(line) - 1];
        } else if (index >= 3 && index < 6) {  // next 3 lines
            char* token = strtok(line, " ");
            token = strtok(NULL, " ");
            unsigned int capacity = atoi(token);
            if (capacity == 0) {
                perror("invalid capacity in config file");
                return 1;
            }

            parkingSpotCapacities[index - 3] = capacity;
        } else if (index >= 6 && index < 9) {  // next 3 lines
            char* token = strtok(line, " ");
            token = strtok(NULL, " ");
            float cost = atof(token);
            if (cost == 0) {
                perror("invalid cost in config file");
                return 1;
            }

            costsPer30Millis[index - 6] = cost;
        }
        index++;
    }
    printf("%s\n", line);

    // last line
    char* token = strtok(line, " ");
    token = strtok(NULL, " ");

    char* logFileName = token;

    // empty the log file
    if ((logFileP = fopen(logFileName, "w")) == NULL) {
        perror("fopen log file");
        return 1;
    }
    fclose(logFileP);

    // determine size of shared memory segments
    int sizeOfParkingSpotGroups = 3 * sizeof(ParkingSpotGroup);
    int sizeOfVesselNodes, sizeOfLedgerVesselNodes;
    if (vesselsNum > 0) {
        sizeOfVesselNodes = 2 * vesselsNum * sizeof(VesselNode);
        sizeOfLedgerVesselNodes = 2 * vesselsNum * sizeof(LedgerVesselNode);
    } else {
        sizeOfVesselNodes = 2 * 4 * sizeof(VesselNode);
        sizeOfLedgerVesselNodes = 2 * 4 * sizeof(LedgerVesselNode);
    }

    // create shared memory segments with different keys
    key_t key = 1;

    int shmIdSharedUtils = shmget(key, sizeof(SharedUtils), 0666 | IPC_CREAT);  // 100: to be changed probably
    if (shmIdSharedUtils == -1) {
        perror("shmget failed");
        return 1;
    }

    key = 2;

    int shmIdParkingSpotGroups = shmget(key, sizeOfParkingSpotGroups, 0666 | IPC_CREAT);  // 100: to be changed probably
    if (shmIdParkingSpotGroups == -1) {
        perror("shmget failed");
        return 1;
    }

    key = 3;

    int shmIdVesselNodes = shmget(key, sizeOfVesselNodes, 0666 | IPC_CREAT);  // 100: to be changed probably
    if (shmIdVesselNodes == -1) {
        perror("shmget failed");
        return 1;
    }

    key = 4;

    int shmIdLedgerNodes = shmget(key, sizeOfLedgerVesselNodes, 0666 | IPC_CREAT);  // 100: to be changed probably
    if (shmIdLedgerNodes == -1) {
        perror("shmget failed");
        return 1;
    }

    // initialize semaphores
    sem_t inOutSem, vesselTypesSem[3], vesselTypesSemOut[3], writeSem, portMasterWakeSem;

    if (sem_init(&inOutSem, 1, 1) == -1) {
        perror("sem_init failed");
        return 1;
    }

    for (unsigned int i = 0; i < 3; i++) {
        if (sem_init(&vesselTypesSem[i], 1, 0) == -1) {  // initialize with 0 because the port-master will handle these semaphores
            perror("sem_init failed");
            return 1;
        }
        if (sem_init(&vesselTypesSemOut[i], 1, 0) == -1) {  // initialize with 0 because the port-master will handle these semaphores
            perror("sem_init failed");
            return 1;
        }
    }

    if (sem_init(&writeSem, 1, 1) == -1) {
        perror("sem_init failed");
        return 1;
    }

    if (sem_init(&portMasterWakeSem, 1, 0) == -1) {  // port-master will wait until the first vessel comes
        perror("sem_init failed");
        return 1;
    }

    // attach shared memory segments
    SharedUtils* sharedUtils = (SharedUtils*)((uint8_t*)shmat(shmIdSharedUtils, 0, 0));
    if (sharedUtils == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }
    ParkingSpotGroup* parkingSpotGroups = (ParkingSpotGroup*)((uint8_t*)shmat(shmIdParkingSpotGroups, 0, 0));
    if (parkingSpotGroups == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }
    VesselNode* vesselNodes = (VesselNode*)((uint8_t*)shmat(shmIdVesselNodes, 0, 0));
    if (vesselNodes == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }
    LedgerVesselNode* ledgerVesselNodes = (LedgerVesselNode*)((uint8_t*)shmat(shmIdLedgerNodes, 0, 0));
    if (ledgerVesselNodes == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }

    // initialize shared utils and parking spot groups
    initSharedUtilsAndParkingSpotGroups(sharedUtils, parkingSpotGroups, parkingSpotTypes, parkingSpotCapacities, costsPer30Millis, inOutSem, vesselTypesSem, vesselTypesSemOut,
                                        sizeOfVesselNodes, sizeOfLedgerVesselNodes, writeSem, portMasterWakeSem, shmIdParkingSpotGroups, shmIdVesselNodes, shmIdLedgerNodes);

    // start port-master and monitor
    int pid1 = fork();
    if (pid1 == 0) {
        execPortMaster(shmIdSharedUtils, logFileName);
    }

    int pid2 = fork();
    if (pid2 == 0) {
        execMonitor(monitorIntervalUsecs, shmIdSharedUtils);
    }

    sleep(1);
    printf("vessels num = %d\n", vesselsNum);
    if (vesselsNum > 0 && spawnIntervalUsecs > 0)
        execRandomVessels(vesselsNum, spawnIntervalUsecs, parkingSpotTypes, shmIdSharedUtils, logFileName);
    else
        execRandomVessels(4, 500000, parkingSpotTypes, shmIdSharedUtils, logFileName);

    // detach shared memory segments
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
    fclose(configFileP);

    int status;
    // wait for port-master to finish
    if (waitpid(pid1, &status, WUNTRACED) == -1) {
        perror("waitpid error");
        exit(EXIT_FAILURE);
    } else {
        printf("port-master exited\n");
    }

    // wait for monitor to finish
    if (waitpid(pid2, &status, WUNTRACED) == -1) {
        perror("waitpid error");
        exit(EXIT_FAILURE);
    } else {
        printf("monitor exited\n");
    }

    struct shmid_ds shmidDs;

    // destroy shared memory segments as they are not needed anymore
    if (shmctl(shmIdSharedUtils, IPC_RMID, &shmidDs) == -1) {
        perror("shmctl failed");
        exit(1);
    }

    if (shmctl(shmIdVesselNodes, IPC_RMID, &shmidDs) == -1) {
        perror("shmctl failed");
        exit(1);
    }

    if (shmctl(shmIdLedgerNodes, IPC_RMID, &shmidDs) == -1) {
        perror("shmctl failed");
        exit(1);
    }

    if (shmctl(shmIdParkingSpotGroups, IPC_RMID, &shmidDs) == -1) {
        perror("shmctl failed");
        exit(1);
    }

    // system("./rmAllShmAndSems.sh");
}