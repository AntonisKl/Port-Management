#ifndef MYPORT_H
#define MYPORT_H

#include "../utils/utils.h"

// handles flags and arguments that are passed to the tty
void handleFlags(int argc, char** argv, char** configFileName, unsigned int* vesselsNum, suseconds_t* vesselIntervalUsecs, suseconds_t* monitorIntervalUsecs);

// executes vessels with random man time and park time (in a specific range)
// vesselsNum: number of vessels to create
// intervalUsecs: time interval in microseconds for each vessel's creation
// vesselTypes: the 3 different types of vessels (e.g. 'S', 'M', 'L')
// shmIdSharedUtils: id for the attachment of the shared utilities
// logFileName: the name of the logging file
void execRandomVessels(unsigned int vesselsNum, suseconds_t intervalUsecs, char* vesselTypes, int shmIdSharedUtils, char* logFileName);

#endif