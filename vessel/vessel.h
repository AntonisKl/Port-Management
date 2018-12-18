#ifndef VESSEL_H
#define VESSEL_H

#include "../utils/utils.h"

void handleFlags(int argc, char** argv, char* type, char* upgradeFlag, suseconds_t* parkTime, suseconds_t* manTime, int* shmId, char** logFileName);

// void getVesselTypeSem(SharedUtils* sharedUtils, char vesselType, sem_t* vesselTypeSemIn /*, sem_t* vesselTypeSemOut*/);

// VesselNode* addVesselNodeToShm(SharedUtils* sharedUtils, char vesselType, char upgradeFlag, suseconds_t parkTime, suseconds_t manTime);

// void getVesselTypeSems(SharedUtils* sharedUtils, char vesselType, sem_t* vesselTypeSemIn, sem_t* vesselTypeSemOut);

VesselNode* addVesselNodeToShmByVesselNode(SharedUtils* sharedUtils, VesselNode* vesselNodes, VesselNode vesselNode);

VesselNode* addVesselNodeToShmByValues(SharedUtils* sharedUtils, VesselNode* vesselNodes, char vesselType, char upgradeFlag, suseconds_t parkTime, suseconds_t manTime, State state);

#endif