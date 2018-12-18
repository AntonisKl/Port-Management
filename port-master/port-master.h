#ifndef PORT_MASTER_H
#define PORT_MASTER_H

#include "../utils/utils.h"

void handleFlags(int argc, char** argv, int* shmId, char** logFileName);

State getNextPendingVesselState(State state);

void addLedgerVesselNode(SharedUtils* sharedUtils, VesselNode* vesselNode, LedgerVesselNode* ledgerVesselNodes, ParkingSpotGroup* parkingSpotGroups, char withUpgrade);

void handleNextVessel(SharedUtils* sharedUtils, VesselNode* vesselNode, LedgerVesselNode* ledgerVesselNodes,
                    ParkingSpotGroup* parkingSpotGroups, unsigned int* sizeOfPendingVesselNodes,
                    unsigned int* nextIndex, VesselNode* pendingVesselNodeRequests[100], FILE* logFileP);

void handleOutGoingVessel(SharedUtils* sharedUtils, VesselNode* vesselNode, ParkingSpotGroup* parkingSpotGroups, LedgerVesselNode* ledgerVesselNodes, unsigned int* nextIndex, char withUpgrade);

void handleIncomingVessel(SharedUtils* sharedUtils, VesselNode* vesselNode, ParkingSpotGroup* parkingSpotGroups,
                        LedgerVesselNode* ledgerVesselNodes, unsigned int* nextIndex,
                        VesselNode* pendingVesselNodeRequests[100], unsigned int* sizeOfPendingVesselNodes, char withUpgrade, FILE* logFileP);
                        
#endif