#ifndef UTILS_H
#define UTILS_H

#include <semaphore.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_STRING_FLOAT_SIZE 20  // including end of string
#define MAX_STRING_INT_SIZE 12    // including end of string

typedef enum State {
    WaitingToEnter,
    Parked,
    WaitingToLeave,
    Completed,
    PendingEnter,
    Entering,
    Leaving
} State;

typedef struct ParkingSpotGroup {
    char type;
    unsigned int maxCapacity;  // how many vessels
    unsigned int curCapacity;
    float costPer30Millis;
} ParkingSpotGroup;

typedef struct LedgerVesselNode {
    int vesselId;  // this will probably be the pid of the vessel process to be distinct for each vessel
    float stayCost;
    char vesselType;
    char upgradeType;  // maybe not used
    long arrivalTime;
    suseconds_t parkTime;
    suseconds_t manTime;
    unsigned long long int waitingTime;
    long departTime;  // this will get value from the port-master after the departure of the vessel
    char withUpgrade;
    unsigned int parkingSpotGroupIndex;  // just a pointer to a ParkingSpotGroup in the shared memory
    State state;
} LedgerVesselNode;

typedef struct VesselNode {
    int vesselId;  // this will probably be the pid of the vessel process to be distinct for each vessel
    char vesselType;
    char withUpgrade;
    suseconds_t parkTime;
    suseconds_t manTime;
    char upgradeType;
    State state;
    suseconds_t waitingTime;
    unsigned int ledgerNodeIndex;  // just a pointer to the corresponding ledger node
} VesselNode;

typedef struct SharedUtils {
    int shmIdPublicLedger, shmIdParkingSpotGroups, shmIdVesselNodes, shmIdLedgerNodes;
    int sizeOfVesselNodes, sizeOfLedgerVesselNodes;
    sem_t inOutSem, vesselTypesSemIn[3], vesselTypesSemPending[3], shmWriteSem, portMasterWakeSem;
    unsigned int queueSize, ledgerSize;
} SharedUtils;

void initParkingSpot(ParkingSpotGroup* parkingSpotGroups, char type, unsigned int maxCapacity, float cost);

void initSharedUtils(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char* parkingSpotTypes,
                      unsigned int* capacities, float* costs, sem_t inOutSem, sem_t vesselTypesSemIn[3], sem_t vesselTypesSemOut[3], int sizeOfVesselNodes,
                      int sizeOfLedgerVesselNodes, sem_t shmWriteSem, sem_t portMasterWakeSem, int shmIdParkingGroups, int shmIdVesselNodes, int shmIdLedgerNodes);

void getVesselTypeSems(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, sem_t* vesselTypeSem, sem_t* vesselTypeSemPending, char vesselType);

void postSemByVesselType(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType);

void postSemPendingByVesselType(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType);

void waitSemByVesselType(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType);

void waitSemPendingByVesselType(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType);

unsigned int getVesselTypeIndex(ParkingSpotGroup* parkingSpotGroups, char vesselType);

sem_t* getVesselTypeSem(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType);

ParkingSpotGroup* getVesselParkingSpotGroup(ParkingSpotGroup parkingSpotGroups[3], char vesselType);

char* vesselStateToString(State state);

suseconds_t caculateWaitingTime(suseconds_t arrivalTime, suseconds_t departTime, suseconds_t manTime, suseconds_t parkTime);

void execPortMaster(int shmId, char* logFileName);

void execMonitor(long statsPrintTime, int shmId, char* logFileName);

void execVessel(char vesselType, char upgradeType, long parkTime, long manTime, int shmId, char* logFileName);

#endif