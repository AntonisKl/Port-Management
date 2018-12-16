#ifndef MYPORT_H
#define MYPORT_H

#include "../utils/utils.h"

void handleFlags(int argc, char** argv, char** configFileName, unsigned int* vesselsNum, suseconds_t* intervalUsecs);

void execRandomVessels(unsigned int vesselsNum, suseconds_t intervalUsecs, char* shipTypes, int shmIdSharedMemory, char* logFileName);

#endif