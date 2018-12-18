#ifndef PORT_MASTER_H
#define PORT_MASTER_H

#include "../utils/utils.h"

// handles flags and arguments
void handleFlags(int argc, char** argv, int* shmId, char** logFileName);

// according to current vessel's state, get the right pending state
State getNextPendingVesselState(State state);

// adds a vessel to the public ledger
void addLedgerVesselNode(SharedUtils* sharedUtils, VesselNode* vesselNode, LedgerVesselNode* ledgerVesselNodes, ParkingSpotGroup* parkingSpotGroups, char withUpgrade);

// handles next vessel that is waiting either in the normal queue or in the pending queue
// nextIndex: index of the current vessel either in the normal queue or in the pending queue (depending on the arguments)
// numOfPendingVesselNodes: number of vessel nodes in the pendingVesselNodeRequests queue
void handleNextVessel(SharedUtils* sharedUtils, VesselNode* vesselNode, LedgerVesselNode* ledgerVesselNodes,
                      ParkingSpotGroup* parkingSpotGroups, unsigned int* numOfPendingVesselNodes,
                      unsigned int* nextIndex, VesselNode** pendingVesselNodeRequests, FILE* logFileP);

// handles outgoing vessel that is waiting either in the normal queue
void handleOutGoingVessel(SharedUtils* sharedUtils, VesselNode* vesselNode, ParkingSpotGroup* parkingSpotGroups, LedgerVesselNode* ledgerVesselNodes, unsigned int* nextIndex, char withUpgrade);

// handles incoming vessel that is waiting either in the normal queue or in the pending queue
// withUpgrade: 0 or 1 | 0-> upgrade not used while handling the vessel, 1-> upgrade used while handling the vessel
void handleIncomingVessel(SharedUtils* sharedUtils, VesselNode* vesselNode, ParkingSpotGroup* parkingSpotGroups,
                          LedgerVesselNode* ledgerVesselNodes, unsigned int* nextIndex,
                          VesselNode* pendingVesselNodeRequests[100], unsigned int* numOfPendingVesselNodes, char withUpgrade, FILE* logFileP);

#endif