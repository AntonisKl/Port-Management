#ifndef PORT_MASTER_H
#define PORT_MASTER_H

#include "../utils/utils.h"

void handleFlags(int argc, char** argv, /* char** chargesFileName,*/ int* shmId, char** logFileName);

void getValuesFromShm(SharedMemory* sharedMemory, sem_t* shipTypesSemIn[3] /*, sem_t* shipTypesSemOut[3]*/, sem_t** inOutSem, sem_t** shmWriteSem,
                      sem_t** portMasterWakeSem, LedgerShipNode** ledgerShipNodes, ShipNode** shipNodesIn /*, ShipNode** shipNodesOut*/,
                      unsigned int* nextInIndex /*, unsigned int* nextOutIndex*/, unsigned int* nextLedgerShipNodeIndex, ParkingSpotGroup* parkingSpotGroups[3]);

State getNextPendingShipState(State state);

LedgerShipNode* addLedgerShipNode(PublicLedger* publicLedger, ShipNode* shipNode, LedgerShipNode* ledgerShipNodes, ParkingSpotGroup* curParkingSpotGroup);

void handleNextShip(SharedMemory* sharedMemory, ShipNode* shipNode, PublicLedger* publicLedger, LedgerShipNode* ledgerShipNodes,
                    ParkingSpotGroup* parkingSpotGroups, unsigned int* sizeOfPendingShipNodes,
                    unsigned int* nextIndex, ShipNode* pendingShipNodeRequests[100], FILE* logFileP);

void handleOutGoingShip(SharedMemory* sharedMemory, ShipNode* shipNode, ParkingSpotGroup* parkingSpotGroups, unsigned int* nextIndex, char withUpgrade);

void handleIncomingShip(SharedMemory* sharedMemory, ShipNode* shipNode, ParkingSpotGroup* parkingSpotGroups, PublicLedger* publicLedger,
                        LedgerShipNode* ledgerShipNodes, unsigned int* nextIndex,
                        ShipNode* pendingShipNodeRequests[100], unsigned int* sizeOfPendingShipNodes, char withUpgrade, FILE* logFileP);
                        
#endif