#ifndef UTILS_H
#define UTILS_H

#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_STRING_FLOAT_SIZE 20 // including end of string
#define MAX_STRING_INT_SIZE 12 // including end of string

typedef struct ShipNode {
    int shipId;  // this will probably be the pid of the vessel process to be distinct for each ship
    float stayCost;
    char shipType;
    suseconds_t arrivalTime;
    suseconds_t parkTimePeriod;
    suseconds_t departTime;  // this will get value from the port-master after the departure of the ship

} ShipNode;

typedef struct ParkingSpot {
    char parkingSpotType;
    unsigned int capacity;  // how many ships
    float costPer30Min;
    char occupied;       // 0 or 1
    ShipNode* shipNode;  // just a pointer to a ShipNode in the shared memory

} ParkingSpot;

typedef struct PublicLedger {
    ParkingSpot* parkingSpots;  // this will be of size 3 according to the excersize description <-- wrong
    ShipNode* shipNodes;        // dynamic array with fixed starting size

} PublicLedger;

void initParkingSpot(ParkingSpot* parkingSpot, char type, unsigned int capacity, float cost);

void initPublicLedger(PublicLedger* publicLedger, char* parkingSpotTypes, unsigned int* capacities, float* costs);

void doShifts(PublicLedger* publicLedger);

#endif