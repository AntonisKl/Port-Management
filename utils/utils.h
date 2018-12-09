#ifndef UTILS_H
#define UTILS_H

#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_STRING_FLOAT_SIZE 20  // including end of string
#define MAX_STRING_INT_SIZE 12    // including end of string

typedef enum State {
    WaitingToEnter,
    Parked,
    WaitingToLeave,
    Completed
} State;

typedef struct ParkingSpotGroup {
    char type;
    unsigned int maxCapacity;  // how many ships
    unsigned int curCapacity;
    float costPer30Min;
    // char occupied;       // 0 or 1
    // ShipNode* shipNode;  // just a pointer to a ShipNode in the shared memory

} ParkingSpotGroup;

typedef struct LedgerShipNode {
    int shipId;  // this will probably be the pid of the vessel process to be distinct for each ship
    float stayCost;
    char shipType;
    char upgradeFlag;  // maybe not used
    suseconds_t arrivalTime;
    suseconds_t parkTimePeriod;
    suseconds_t departTime;                      // this will get value from the port-master after the departure of the ship
    ParkingSpotGroup* parkingSpotGroupOccupied;  // just a pointer to a ParkingSpotGroup in the shared memory

} LedgerShipNode;

typedef struct ShipNode {
    int shipId;  // this will probably be the pid of the vessel process to be distinct for each ship
    // float stayCost;
    char shipType;
    // suseconds_t arrivalTime;
    suseconds_t parkTimePeriod;
    suseconds_t manTimePeriod;
    char upgradeFlag;
    State state;
    // suseconds_t departTime;                      // this will get value from the port-master after the departure of the ship
    // ParkingSpotGroup* parkingSpotGroupOccupied;  // just a pointer to a ParkingSpotGroup in the shared memory

} ShipNode;

typedef struct PublicLedger {
    LedgerShipNode* ledgerShipNodes;  // dynamic array with fixed starting size
    unsigned int nextShipNodeIndex;

} PublicLedger;

typedef struct SharedMemory {
    // char shipTypes[3];
    int sizeOfShipNodes, sizeOfLedgerShipNodes;
    sem_t inOutSem, shipTypesSemIn[3], shipTypesSemOut[3], writeSem;
    PublicLedger* publicLedger;
    ParkingSpotGroup* parkingSpotGroups;  // this will be of size 3 according to the excersize description <-- wrong
    ShipNode* shipNodes;
    ShipNode** outShipNodes;
    unsigned int nextShipNodeIndex, nextOutShipNodeIndex;

} SharedMemory;

void initParkingSpot(ParkingSpotGroup* parkingSpotGroups, char type, unsigned int maxCapacity, float cost);

void initPublicLedger(SharedMemory* sharedMemory, char* parkingSpotTypes, unsigned int* capacities, float* costs, sem_t inOutSem,
                      sem_t shipTypesSemIn[3], sem_t shipTypesSemOut[3], int sizeOfShipNodes, int sizeOfLedgerShipNodes, sem_t writeSem);

void doShifts(SharedMemory* sharedMemory, int sizeOfShipNodes, int sizeOfLedgerShipNodes);

#endif