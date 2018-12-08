typedef struct PublicLedgerNode {

	int shipId; // this will probably be the pid of the vessel process to be distinct for each ship
	float stayCost;
	char shipType;
	suseconds_t arrivalTime;
	suseconds_t parkTimePeriod;
	suseconds_t departTime; // this will get value from the port-master after the departure of the ship

} PublicLedgerNode;

typedef struct ParkingSpot {

	char parkingSpotType;
	char* shipTypesCompatible;
	float costPer30Min;

} ParkingSpot;

typedef struct PublicLedger {

	ParkingSpot* parkingSpots; // this will be of size 3 according to the excersize description
	PublicLedgerNode* publicLedgerNodes; // dynamic array with fixed starting size

} PublicLedger;
