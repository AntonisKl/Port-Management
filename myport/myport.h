#ifndef MYPORT_H
#define MYPORT_H

#include "../utils/utils.h"

void handleFlags(int argc, char** argv, char** configFileName, unsigned int* vesselsNum, suseconds_t* vesselIntervalUsecs, suseconds_t* monitorIntervalUsecs);

void execRandomVessels(unsigned int vesselsNum, suseconds_t intervalUsecs, char* vesselTypes, int shmIdSharedUtils, char* logFileName);

#endif