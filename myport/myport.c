#include "myport.h"

void handleFlags(int argc, char** argv, char** configFileName) {
    if (argc != 3) {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }

    char* ptr;

    if (strcmp(argv[1], "-l") == 0) {
        (*configFileName) = argv[2];
    } else {
        printf("Invalid flags\nExiting...\n");
        exit(1);
    }
}

int main(int argc, char** argv) {
    char* configFileName;
    char parkingSpotTypes[3];
    float costsPer30minutes[3];
    unsigned int parkingSpotCapacities[3];

    handleFlags(argc, argv, &configFileName);

    FILE* configFileP;
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

    int sizeOfParkingSpotGroups = 3 * sizeof(ParkingSpotGroup);
    int sizeOfShipNodes = 10 * (parkingSpotCapacities[0] + parkingSpotCapacities[1] + parkingSpotCapacities[2]) * sizeof(ShipNode);
    int sizeOfLedgerShipNodes = 10 * (parkingSpotCapacities[0] + parkingSpotCapacities[1] + parkingSpotCapacities[2]) * sizeof(LedgerShipNode);

    // ftok to generate unique key
    key_t key = ftok("shm", 65);

    // shmget returns an identifier in shmid
    int shmid = shmget(key, sizeof(SharedMemory) + sizeof(PublicLedger) + sizeOfParkingSpotGroups + sizeOfShipNodes + sizeOfLedgerShipNodes, 0666 | IPC_CREAT);  // 100: to be changed probably
    if (shmid == -1) {
        perror("shmget failed");
        return 1;
    }

    sem_t inOutSem, shipTypesSem[3];

    if (sem_init(&inOutSem, 1, 1) == -1) {
        perror("sem_init failed");
        return 1;
    }

    for (unsigned int i = 0; i < 3; i++) {
        if (sem_init(&shipTypesSem[i], 1, 1) == -1) {
            perror("sem_init failed");
            return 1;
        }
    }

    SharedMemory* sharedMemory = (SharedMemory*)shmat(shmid, NULL, 0);
    doShifts(sharedMemory, sizeOfShipNodes);  // do necessary shifts
    initPublicLedger(sharedMemory, parkingSpotTypes, parkingSpotCapacities, costsPer30minutes, inOutSem, shipTypesSem, sizeOfShipNodes, sizeOfLedgerShipNodes);

    // start monitor and port-master

    fclose(configFileP);
}