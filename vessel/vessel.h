#ifndef VESSEL_H
#define VESSEL_H

#include "../utils/utils.h"

void handleFlags(int argc, char** argv, char* type, char* upgradeFlag, suseconds_t* parkTime, suseconds_t* manTime, int* shmId, char** logFileName);

sem_t* getShipTypeSem(SharedMemory* sharedMemory, char shipType);

#endif