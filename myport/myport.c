#include "myport.h"

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
        unsigned int upgradeIndex = rand() % 3;
        long parkTimeMillis = (rand() % 500) + 1;
        long manTimePeriodMillis = (rand() % 500) + 1;

        if (fork() == 0) {
            execVessel(vesselTypes[typeIndex], vesselTypes[upgradeIndex], parkTimeMillis, manTimePeriodMillis, shmIdSharedUtils, logFileName);
        }
        usleep(intervalUsecs);
    }
}

int main(int argc, char** argv) {
    char* configFileName;
    char parkingSpotTypes[3];
    float costsPer30minutes[3];
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
        if (index >= 0 && index < 3)
            parkingSpotTypes[index] = line[strlen(line) - 1];
        else if (index >= 3 && index < 6) {
            char* token = strtok(line, " ");
            token = strtok(NULL, " ");
            unsigned int capacity = atoi(token);
            if (capacity == 0) {
                perror("invalid capacity in config file");
                return 1;
            }

            parkingSpotCapacities[index - 3] = capacity;
        } else if (index >= 6 && index < 9) {
            char* token = strtok(line, " ");
            token = strtok(NULL, " ");
            float cost = atof(token);
            if (cost == 0) {
                perror("invalid cost in config file");
                return 1;
            }

            costsPer30minutes[index - 6] = cost;
        }
        index++;
    }
    printf("%s\n", line);

    char* token = strtok(line, " ");
    token = strtok(NULL, " ");

    char* logFileName = token;

    if ((logFileP = fopen(logFileName, "w")) == NULL) {
        perror("fopen log file");
        return 1;
    }
    fclose(logFileP);

    int sizeOfParkingSpotGroups = 3 * sizeof(ParkingSpotGroup);
    int sizeOfVesselNodes, sizeOfLedgerVesselNodes;
    if (vesselsNum > 0) {
        sizeOfVesselNodes = 2 * vesselsNum * sizeof(VesselNode);
        sizeOfLedgerVesselNodes = 2 * vesselsNum * sizeof(LedgerVesselNode);
    } else {
        sizeOfVesselNodes = 2 * 4 * sizeof(VesselNode);
        sizeOfLedgerVesselNodes = 2 * 4 * sizeof(LedgerVesselNode);
    }

    key_t key = 1;

    int shmIdSharedUtils = shmget(key, sizeof(SharedUtils), 0666 | IPC_CREAT);  // 100: to be changed probably
    if (shmIdSharedUtils == -1) {
        perror("shmget failed");
        return 1;
    }

    key = 2;

    int shmIdParkingGroups = shmget(key, sizeOfParkingSpotGroups, 0666 | IPC_CREAT);  // 100: to be changed probably
    if (shmIdParkingGroups == -1) {
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

    sem_t inOutSem, vesselTypesSemIn[3], vesselTypesSemOut[3], writeSem, portMasterWakeSem;

    if (sem_init(&inOutSem, 1, 1) == -1) {
        perror("sem_init failed");
        return 1;
    }

    for (unsigned int i = 0; i < 3; i++) {
        if (sem_init(&vesselTypesSemIn[i], 1, 0) == -1) {  // initialize with 0 because the port-master will handle these semaphores
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

    SharedUtils* sharedUtils = (SharedUtils*)((uint8_t*)shmat(shmIdSharedUtils, 0, 0));  // error checking add <-------------------
    ParkingSpotGroup* parkingSpotGroups = (ParkingSpotGroup*)((uint8_t*)shmat(shmIdParkingGroups, 0, 0));
    VesselNode* vesselNodes = (VesselNode*)((uint8_t*)shmat(shmIdVesselNodes, 0, 0));
    LedgerVesselNode* ledgerVesselNodes = (LedgerVesselNode*)((uint8_t*)shmat(shmIdLedgerNodes, 0, 0));

    initSharedUtils(sharedUtils, parkingSpotGroups, parkingSpotTypes, parkingSpotCapacities, costsPer30minutes, inOutSem, vesselTypesSemIn, vesselTypesSemOut,
                    sizeOfVesselNodes, sizeOfLedgerVesselNodes, writeSem, portMasterWakeSem, shmIdParkingGroups, shmIdVesselNodes, shmIdLedgerNodes);

    // start monitor and port-master
    int pid = fork();
    if (pid == 0) {
        execPortMaster(shmIdSharedUtils, logFileName);
    }

    if (fork() == 0) {
        execMonitor(monitorIntervalUsecs, shmIdSharedUtils, logFileName);
    }

    sleep(1);
    printf("vessels num = %d\n", vesselsNum);
    if (vesselsNum > 0 && spawnIntervalUsecs > 0)
        execRandomVessels(vesselsNum, spawnIntervalUsecs, parkingSpotTypes, shmIdSharedUtils, logFileName);
    else
        execRandomVessels(4, 500000, parkingSpotTypes, shmIdSharedUtils, logFileName);

    shmdt(sharedUtils);
    shmdt(parkingSpotGroups);
    shmdt(vesselNodes);
    shmdt(ledgerVesselNodes);

    int status;
    if (waitpid(pid, &status, WUNTRACED) == -1) {
        perror("waitpid error");
        exit(EXIT_FAILURE);
    } else {
        printf("port-master exited\n");
    }

    fclose(configFileP);
}