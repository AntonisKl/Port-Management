#ifndef VESSEL_H
#define VESSEL_H

#include "../utils/utils.h"

void handleFlags(int argc, char** argv, char* type, char* upgradeFlag, suseconds_t* parkTime, suseconds_t* manTime, int* shmId, char** logFileName);

// void getShipTypeSem(SharedMemory* sharedMemory, char shipType, sem_t* shipTypeSemIn /*, sem_t* shipTypeSemOut*/);

// ShipNode* addShipNodeToShm(SharedMemory* sharedMemory, char shipType, char upgradeFlag, suseconds_t parkTime, suseconds_t manTime);

// void getShipTypeSems(SharedMemory* sharedMemory, char shipType, sem_t* shipTypeSemIn, sem_t* shipTypeSemOut);

ShipNode* addShipNodeToShmByShipNode(SharedMemory* sharedMemory, ShipNode* shipNodes, ShipNode shipNode);

ShipNode* addShipNodeToShmByValues(SharedMemory* sharedMemory, ShipNode* shipNodes, char shipType, char upgradeFlag, suseconds_t parkTimePeriod, suseconds_t manTimePeriod, State state);

#endif