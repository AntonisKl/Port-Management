#include "myport.h"

void handleFlags(int argc, char** argv, char** configFileName, unsigned int* vesselsNum, suseconds_t* intervalUsecs) {
    if (argc != 3 && argc != 7) {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    // char* ptr;

    if (strcmp(argv[1], "-l") == 0) {
        (*configFileName) = argv[2];
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    if (argc == 7) {
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
            (*intervalUsecs) = atol(argv[6]) * 1000;
            if ((*intervalUsecs) < 0) {
                printf("Invalid flags\nExiting...\n");
                exit(1);
            }
        } else {
            printf("Invalid flags\nExiting...\n");
            exit(1);
        }
    }
}

void execRandomVessels(unsigned int vesselsNum, suseconds_t intervalUsecs, char* shipTypes, int shmIdSharedMemory, char* logFileName) {
    srand(time(NULL));

    for (unsigned int i = 0; i < vesselsNum; i++) {
        // printf("vessel creation\n");
        if (fork() == 0) {
            unsigned int typeIndex = rand() % 3;
            unsigned int upgradeIndex = rand() % 3;
            // long manTimePeriodMillis = (rand() % 5000) + 1, parkTimePeriodMillis = (rand() % 5000) + 1;

            // printf("type index: %u, uIndex: %u\n", typeIndex, upgradeIndex);
            // printf("exev vessel\n");
            execVessel(shipTypes[typeIndex], shipTypes[upgradeIndex], 1500, 500, shmIdSharedMemory, logFileName);
        }
        usleep(intervalUsecs);
    }
}

int main(int argc, char** argv) {
    char* configFileName;
    char parkingSpotTypes[3];
    float costsPer30minutes[3];
    unsigned int parkingSpotCapacities[3], vesselsNum = -1;
    suseconds_t intervalUsecs = -1;

    handleFlags(argc, argv, &configFileName, &vesselsNum, &intervalUsecs);

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
    // fprintf(logFileP, "\0"); // empty file
    fclose(logFileP);

    int sizeOfParkingSpotGroups = 3 * sizeof(ParkingSpotGroup);
    int sizeOfShipNodes = 10 * (parkingSpotCapacities[0] + parkingSpotCapacities[1] + parkingSpotCapacities[2]) * sizeof(ShipNode);
    int sizeOfLedgerShipNodes = 10 * (parkingSpotCapacities[0] + parkingSpotCapacities[1] + parkingSpotCapacities[2]) * sizeof(LedgerShipNode);
    // int sizeOfOutShipNodes = 10 * (parkingSpotCapacities[0] + parkingSpotCapacities[1] + parkingSpotCapacities[2]) * sizeof(ShipNode*);

    // ftok to generate unique key
    key_t key = 1;

    // shmget returns an identifier in shmid
    int shmIdSharedMemory = shmget(key, sizeof(SharedMemory), 0666 | IPC_CREAT);  // 100: to be changed probably
    if (shmIdSharedMemory == -1) {
        perror("shmget failed");
        return 1;
    }

    key = 2;

    // shmget returns an identifier in shmid
    int shmIdPublicLedger = shmget(key, sizeof(PublicLedger), 0666 | IPC_CREAT);  // 100: to be changed probably
    if (shmIdPublicLedger == -1) {
        perror("shmget failed");
        return 1;
    }

    key = 3;

    // shmget returns an identifier in shmid
    int shmIdParkingGroups = shmget(key, sizeOfParkingSpotGroups, 0666 | IPC_CREAT);  // 100: to be changed probably
    if (shmIdParkingGroups == -1) {
        perror("shmget failed");
        return 1;
    }

    key = 4;

    // shmget returns an identifier in shmid
    int shmIdShipNodes = shmget(key, sizeOfShipNodes, 0666 | IPC_CREAT);  // 100: to be changed probably
    if (shmIdShipNodes == -1) {
        perror("shmget failed");
        return 1;
    }

    key = 5;

    // shmget returns an identifier in shmid
    int shmIdLedgerNodes = shmget(key, sizeOfLedgerShipNodes, 0666 | IPC_CREAT);  // 100: to be changed probably
    if (shmIdLedgerNodes == -1) {
        perror("shmget failed");
        return 1;
    }

    sem_t inOutSem, shipTypesSemIn[3], shipTypesSemOut[3], writeSem, portMasterWakeSem;

    if (sem_init(&inOutSem, 1, 1) == -1) {
        perror("sem_init failed");
        return 1;
    }

    for (unsigned int i = 0; i < 3; i++) {
        if (sem_init(&shipTypesSemIn[i], 1, 0) == -1) {  // initialize with 0 because the port-master will handle these semaphores
            perror("sem_init failed");
            return 1;
        }
        if (sem_init(&shipTypesSemOut[i], 1, 0) == -1) {  // initialize with 0 because the port-master will handle these semaphores
            perror("sem_init failed");
            return 1;
        }
    }

    if (sem_init(&writeSem, 1, 1) == -1) {
        perror("sem_init failed");
        return 1;
    }

    if (sem_init(&portMasterWakeSem, 1, 0) == -1) {  // port-master will wait until the first ship comes
        perror("sem_init failed");
        return 1;
    }

    SharedMemory* sharedMemory = (SharedMemory*)((uint8_t*)shmat(shmIdSharedMemory, 0, 0));  // error checking add <-------------------
    PublicLedger* publicLedger = (PublicLedger*)((uint8_t*)shmat(shmIdPublicLedger, 0, 0));
    ParkingSpotGroup* parkingSpotGroups = (ParkingSpotGroup*)((uint8_t*)shmat(shmIdParkingGroups, 0, 0));
    ShipNode* shipNodes = (ShipNode*)((uint8_t*)shmat(shmIdShipNodes, 0, 0));
    LedgerShipNode* ledgerShipNodes = (LedgerShipNode*)((uint8_t*)shmat(shmIdLedgerNodes, 0, 0));

    // doShifts(sharedMemory, sizeOfShipNodes, sizeOfLedgerShipNodes);  // do necessary shifts
    initPublicLedger(sharedMemory, publicLedger, parkingSpotGroups, parkingSpotTypes, parkingSpotCapacities, costsPer30minutes, inOutSem, shipTypesSemIn, shipTypesSemOut,
                     sizeOfShipNodes, sizeOfLedgerShipNodes, writeSem, portMasterWakeSem, shmIdPublicLedger, shmIdParkingGroups, shmIdShipNodes, shmIdLedgerNodes);

    // start monitor and port-master
    int pid = fork();
    if (pid == 0)
        execPortMaster(shmIdSharedMemory, logFileName);

    sleep(1);
    // if (fork() == 0)
    //     execVessel(parkingSpotTypes[0], parkingSpotTypes[2], 1500, 500, shmIdSharedMemory, logFileName);
    // // sleep(2);

    // if (fork() == 0)
    //     execVessel(parkingSpotTypes[0], parkingSpotTypes[1], 1500, 500, shmIdSharedMemory, logFileName);

    if (vesselsNum > 0 && intervalUsecs > 0)
        execRandomVessels(vesselsNum, intervalUsecs, parkingSpotTypes, shmIdSharedMemory, logFileName);
    else
        execRandomVessels(4, 500000, parkingSpotTypes, shmIdSharedMemory, logFileName);

    shmdt(sharedMemory);
    shmdt(publicLedger);
    shmdt(parkingSpotGroups);
    shmdt(shipNodes);
    shmdt(ledgerShipNodes);

    // wait(NULL);
    // wait(NULL);
    // wait(NULL);
    int status;
    if (waitpid(pid, &status, WUNTRACED) == -1) {
        perror("waitpid error");
        exit(EXIT_FAILURE);
    } else {
        printf("port-master exited\n");
    }

    fclose(configFileP);
    // shmctl(shmid, IPC_RMID, NULL);
}