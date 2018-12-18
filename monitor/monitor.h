#ifndef MONITOR_H
#define MONITOR_H

#include "../utils/utils.h"

void handleFlags(int argc, char** argv, suseconds_t* statsPrintTimeUsecs, int* shmId, char** logFileName);

#endif