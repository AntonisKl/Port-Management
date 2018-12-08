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

    int sizeOfParkingSpots = 3 * sizeof(ParkingSpot);
    int sizeOfShipNodes = 10 * (parkingSpotCapacities[0] + parkingSpotCapacities[1] + parkingSpotCapacities[2]) * sizeof(ShipNode);

    // ftok to generate unique key
    key_t key = ftok("shm", 65);

    // shmget returns an identifier in shmid
    int shmid = shmget(key, sizeof(PublicLedger) + sizeOfParkingSpots + sizeOfShipNodes, 0666 | IPC_CREAT);  // 100: to be changed probably
    if (shmid == -1) {
        perror("shmget failed");
        return 1;
    }

    PublicLedger* publicLedger = (PublicLedger*)shmat(shmid, NULL, 0);
    doShifts(publicLedger); // do necessary shifts
    initPublicLedger(publicLedger, parkingSpotTypes, parkingSpotCapacities, costsPer30minutes);

    fclose(configFileP);
}