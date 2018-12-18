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

// different states of vessels
typedef enum State {
    WaitingToEnter,  // vessel waiting in the normal queue to enter the port
    Parked,          // vessel parked in the port
    WaitingToLeave,  // vessel waiting in the normal queue to leave the port
    Completed,       // vessel left the port completely
    PendingEnter,    // vessel waiting in the pending queue to enter the port
    Entering,        // vessel started entering the port
    Leaving          // vessel started leaving the port
} State;

// group of parking spots that correspond to one specific type of vessel
typedef struct ParkingSpotGroup {
    char type;
    unsigned int maxCapacity;
    unsigned int curCapacity;
    float costPer30Millis;
} ParkingSpotGroup;

// node of the Public Ledger (vessel's process does not have access to this kind of nodes)
typedef struct LedgerVesselNode {
    int vesselId;      // this is the pid of the vessel which is distinct for each vessel
    float stayCost;    // total cost of vessel's parking time period
    char vesselType;   // one of the 3 types that are specified in the config.txt file
    char upgradeType;  // type in which the vessel wants to upgrade (if it is a type that is smaller or equal to the vessel's then we do not consider the vessel upgradeable)
    long arrivalTime;  // time in which the vessel is added to the ledger (entering the port)
    long departTime;   // time in which the vessel is starting to leave the port
    suseconds_t parkTime;
    suseconds_t manTime;
    unsigned long long int waitingTime;  // total waiting time of the vessel (it is updated only when the vessel leaves the port)
    char withUpgrade;                    // if it is 0: vessel was handled with its upgrade, if it is 1: vessel was handled without its upgrade (if it had one)
    unsigned int parkingSpotGroupIndex;  // used to find the occupied parking spot group
    State state;
} LedgerVesselNode;

// node of the main queue for incoming and outgoing vessels
typedef struct VesselNode {
    int vesselId;  // this is the pid of the vessel which is distinct for each vessel
    char vesselType;
    char withUpgrade;
    suseconds_t parkTime;
    suseconds_t manTime;
    char upgradeType;
    State state;
    suseconds_t waitingTime;       // waiting time of vessel so far
    unsigned int ledgerNodeIndex;  // just a pointer to the corresponding ledger node
} VesselNode;

// useful variables that are shared between all processes
typedef struct SharedUtils {
    int shmIdPublicLedger, shmIdParkingSpotGroups, shmIdVesselNodes, shmIdLedgerNodes;            // necessary for shared memory attachment
    int sizeOfVesselNodes, sizeOfLedgerVesselNodes;                                               // size of each shared memory segment in bytes
    sem_t inOutSem, vesselTypesSem[3], vesselTypesSemPending[3], shmWriteSem, portMasterWakeSem;  // necessary semaphores for IPC
    // inOutSem: for only one vessel to be able to get in and out of the port each time. This semaphore is being sem_waited only by the port-master and posted only by vessels
    // vesselTypesSem[3]: 3 semaphores, one for each vessel type. They are used to let vessels that are waiting in the normal queue to get in and out of the port
    // vesselTypesSemPending[3]: 3 semaphores, one for each vessel type. They are used to let vessels that are waiting in the pending queue to get in the port
    // shmWriteSem: for only one vessel to be able to add its node to the shared memory each time
    // portMasterWakeSem: vessels are using this semaphore to wake up the port-master who is sem_waiting on this semaphore while there is no job to do
    unsigned int queueSize, ledgerSize;  // queueSize: number of vessel nodes currently in the queue, ledgerSize: number of ledger nodes currently in the public ledger
} SharedUtils;

// initializes a group of parking spots
void initParkingSpot(ParkingSpotGroup* parkingSpotGroups, char type, unsigned int maxCapacity, float cost);

// initializes shared utilities and groups of parking spots
void initSharedUtilsAndParkingSpotGroups(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char* parkingSpotTypes,
                                         unsigned int* capacities, float* costs, sem_t inOutSem, sem_t vesselTypesSem[3], sem_t vesselTypesSemOut[3], int sizeOfVesselNodes,
                                         int sizeOfLedgerVesselNodes, sem_t shmWriteSem, sem_t portMasterWakeSem, int shmIdParkingGroups, int shmIdVesselNodes, int shmIdLedgerNodes);

// posts the semaphore that corresponds to the vesselType
void postSemByVesselType(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType);

// posts the pending semaphore that corresponds to the vesselType
void postSemPendingByVesselType(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType);

// waits on the semaphore that corresponds to the vesselType
void waitSemByVesselType(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType);

// waits on the pending semaphore that corresponds to the vesselType
void waitSemPendingByVesselType(SharedUtils* sharedUtils, ParkingSpotGroup* parkingSpotGroups, char vesselType);

// returns the index of parkingSpotGroups table that corresponds to the vesselType
unsigned int getVesselTypeIndex(ParkingSpotGroup* parkingSpotGroups, char vesselType);

// returns the string that corresponds to the given state (used for printing purposes only)
char* vesselStateToString(State state);

// handles the necessary arguments executes port-master
void execPortMaster(int shmId, char* logFileName);

// handles the necessary arguments executes monitor
void execMonitor(long statsPrintTime, int shmId, char* logFileName);

// handles the necessary arguments executes vessel
void execVessel(char vesselType, char upgradeType, long parkTime, long manTime, int shmId, char* logFileName);

#endif