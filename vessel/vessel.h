#ifndef VESSEL_H
#define VESSEL_H

#include "../utils/utils.h"

// handles flags and arguments
void handleFlags(int argc, char** argv, char* type, char* upgradeFlag, suseconds_t* parkTime, suseconds_t* manTime, int* shmId, char** logFileName);

// adds a vessel node to the vesselNodes shared memory segment and from values of an already existing vessel node
VesselNode* addVesselNodeToShmByVesselNode(SharedUtils* sharedUtils, VesselNode* vesselNodes, VesselNode vesselNode);

// adds a vessel node to the vesselNodes shared memory segment from values passed as arguments
VesselNode* addVesselNodeToShmByValues(SharedUtils* sharedUtils, VesselNode* vesselNodes, char vesselType, char upgradeFlag, suseconds_t parkTime, suseconds_t manTime, State state);

#endif