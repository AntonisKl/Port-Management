#ifndef UTILS_H
#define UTILS_H

#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_STRING_FLOAT_SIZE 20  // including end of string
#define MAX_STRING_INT_SIZE 12    // including end of string

typedef enum State {
    WaitingToEnter,
    Parked,
    WaitingToLeave,
    Completed,
    PendingEnter,
    PendingLeave
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
    suseconds_t manTimePeriod;
    suseconds_t departTime;                      // this will get value from the port-master after the departure of the ship
    ParkingSpotGroup* parkingSpotGroupOccupied;  // just a pointer to a ParkingSpotGroup in the shared memory
    State state;

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
    LedgerShipNode* ledgerShipNode;  // just a pointer to the corresponding ledger node
    // suseconds_t departTime;                      // this will get value from the port-master after the departure of the ship
    // ParkingSpotGroup* parkingSpotGroupOccupied;  // just a pointer to a ParkingSpotGroup in the shared memory

} ShipNode;

typedef struct PublicLedger {
    LedgerShipNode* ledgerShipNodes;  // dynamic array with fixed starting size
    unsigned int size;

} PublicLedger;

typedef struct SharedMemory {
    // char shipTypes[3];
    int shmIdPublicLedger, shmIdParkingSpotGroups, shmIdShipNodes, shmIdLedgerNodes;
    int sizeOfShipNodes, sizeOfLedgerShipNodes;
    sem_t inOutSem, shipTypesSemIn[3], shipTypesSemPending[3], shmWriteSem, portMasterWakeSem;
    PublicLedger* publicLedger;
    ParkingSpotGroup* parkingSpotGroups;  // this will be of size 3 according to the excersize description <-- wrong
    ShipNode* shipNodes;
    // ShipNode** shipNodesOut;
    unsigned int sizeIn /*, sizeOut*/;

} SharedMemory;

void initParkingSpot(ParkingSpotGroup* parkingSpotGroups, char type, unsigned int maxCapacity, float cost);

void initPublicLedger(SharedMemory* sharedMemory, PublicLedger* publicLedger, ParkingSpotGroup* parkingSpotGroups, char* parkingSpotTypes,
                      unsigned int* capacities, float* costs, sem_t inOutSem, sem_t shipTypesSemIn[3], sem_t shipTypesSemOut[3], int sizeOfShipNodes,
                      int sizeOfLedgerShipNodes, sem_t shmWriteSem, sem_t portMasterWakeSem, int shmIdPublicLedger, int shmIdParkingGroups, int shmIdShipNodes, int shmIdLedgerNodes);

void doShifts(SharedMemory* sharedMemory, int sizeOfShipNodes, int sizeOfLedgerShipNodes);

void getShipTypeSems(SharedMemory* sharedMemory, ParkingSpotGroup* parkingSpotGroups, sem_t* shipTypeSem, sem_t* shipTypeSemPending, char shipType);

void postSemByShipType(SharedMemory* sharedMemory, ParkingSpotGroup* parkingSpotGroups, char shipType);

void postSemPendingByShipType(SharedMemory* sharedMemory, ParkingSpotGroup* parkingSpotGroups, char shipType);

void waitSemByShipType(SharedMemory* sharedMemory, ParkingSpotGroup* parkingSpotGroups, char shipType);

void waitSemPendingByShipType(SharedMemory* sharedMemory, ParkingSpotGroup* parkingSpotGroups, char shipType);

unsigned int getShipTypeIndex(ParkingSpotGroup* parkingSpotGroups, char shipType);

sem_t* getShipTypeSem(SharedMemory* sharedMemory, ParkingSpotGroup* parkingSpotGroups, char shipType);

ParkingSpotGroup* getShipParkingSpotGroup(ParkingSpotGroup parkingSpotGroups[3], char shipType);

void execPortMaster(int shmId, char* logFileName);

void execMonitor(suseconds_t statusPrintTime, suseconds_t statsPrintTime, int shmId, char* logFileName);

void execVessel(char shipType, char upgradeFlag /* 0 or 1 */, long parkTime, long manTime, int shmId, char* logFileName);

#endif