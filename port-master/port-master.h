#ifndef PORT_MASTER_H
#define PORT_MASTER_H

#include "../utils/utils.h"

void handleFlags(int argc, char** argv, /* char** chargesFileName,*/ int* shmId, char** logFileName);

void getValuesFromShm(SharedMemory* sharedMemory, sem_t* shipTypesSemIn[3], sem_t* shipTypesSemOut[3], sem_t* inOutSem, sem_t* shmWriteSem,
                      LedgerShipNode** ledgerShipNodes, ShipNode** shipNodesIn, ShipNode** shipNodesOut, unsigned int* nextShipNodeInIndex, unsigned int* nextShipNodeOutIndex,
                      unsigned int* nextLedgerShipNodeIndex, ParkingSpotGroup* parkingSpotGroups[3]);

LedgerShipNode* addLedgerShipNode(PublicLedger* publicLedger, ShipNode* shipNode, ParkingSpotGroup* curParkingSpotGroup);

#endif