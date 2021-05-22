/* 046267 Computer Architecture - Spring 2021 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>

#define ADDRESS_SIZE 32
#define PTWO2(x) (1 << (x))
#define PTWO(x) (pow(2, (x)))
#define DEBUG if(debug) cout << "[DEBUG] "
bool debug = false;

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

// cahce entry - a cache line
struct cacheEntry {
	uint32_t tag;
	int valid;
	int dirty;
	uint32_t address;
};

void printEntry(cacheEntry x) {
	cout << x.valid << " | " << x.dirty << " | " << x.tag << endl;
}

// cache class
class cache {
	// parameters:
	int size;
	int assoc;
	int cycles;
	
	// field sizes
	int offsetSize;
	int memCycles;
	bool writeAlloc;
	int setSize;
	int tagSize;
	// ways 2darray of cache entries
	cacheEntry** ways;
	// lru array 
	int** lru;
	
public:

	// counters 
	int timeAccu;
	int missCounter;
	int accessCounter;

	// cache level pointers 
	cache* below;
	cache* above;

	cache(unsigned size, unsigned assoc, unsigned cycles, unsigned blockSize, unsigned memCycles, unsigned writeAlloc){
		this->size = size;
		this->assoc = assoc;
		this->cycles = cycles;

		this->timeAccu = 0;
		this->missCounter = 0;
		this->accessCounter = 0;

		this->offsetSize = blockSize;
		this->memCycles = memCycles;
		this->writeAlloc = writeAlloc == 1;
		this->setSize = this->size - this->offsetSize - this->assoc;
		this->tagSize = ADDRESS_SIZE - this->setSize - this->offsetSize;

		// allocating ways array
		this->ways = new cacheEntry * [PTWO(this->assoc)];
		for (int way = 0; way < PTWO(this->assoc); way++) {
			this->ways[way] = new cacheEntry[PTWO(this->setSize)];
			for (int setnum = 0; setnum < PTWO(this->setSize); setnum++) {
				this->ways[way][setnum].dirty = 0;
				this->ways[way][setnum].valid = 0;
				this->ways[way][setnum].tag = 0;
				this->ways[way][setnum].address = 0;
			}
		}

		// allocating and initializing lru array
		this->lru = new int*[PTWO(this->setSize)];
		for (int i = 0; i < PTWO(this->setSize); i++) {
			lru[i] = new int[PTWO(this->assoc)];
			for (int j = 0; j < PTWO(this->assoc); j++) {
				lru[i][j] = j;
			}
		}
	}
	~cache() {
		// deallocating ways array
		for (int way = 0; way < PTWO(this->assoc); way++) {
			delete[] this->ways[way];
		}
		delete[] ways;

		// deallocating lru array
		for (int i = 0; i < PTWO(this->setSize); i++) {
			delete[] lru[i];
		}
		delete[] lru;
	}

	/*void printWays() {
		for (int way = 0; way < PTWO(assoc); way++) {
			cout << "way " << way << ": " << endl;
			for (int setnum)
		}
	}*/

	void getWayLruByAddress(uint32_t address, int* _way, uint32_t* _setnum) {
		uint32_t offset, setnum, tag;
		getFromAddress(address, &offset, &setnum, &tag);
		for (int way = 0; way < PTWO(assoc); way++) {
			if (ways[way][setnum].tag == tag) {
				*_way = way;
				*_setnum = setnum;
			}
		}
	}

	void updateLru(int setnum, int way) {
		int x = lru[setnum][way];
		lru[setnum][way] = PTWO(assoc) - 1;
		for (int j = 0; j < PTWO(assoc); j++) {
			if (j != way && lru[setnum][j] > x) {
				lru[setnum][j] -= 1;
			}
		}
	}
	
	int getLruMin(int setnum) {
		for (int i = 0; i < PTWO(assoc); i++)
			if (lru[setnum][i] == 0)
				return i;
		return -1;
	}

	int search(uint32_t address) {
		uint32_t offset, setnum, tag;
		getFromAddress(address, &offset, &setnum, &tag);

		DEBUG << "assoc " << assoc << " " << PTWO(assoc) << endl;
		for (int way = 0; way < PTWO(assoc); way++) {
			DEBUG << "way: " << way << " " << ways[way][setnum].tag << " " << ways[way][setnum].valid << endl;
			if (ways[way][setnum].tag == tag && ways[way][setnum].valid == 1) {
				return way;
			}
		}
		return -1;
	}

	void getFromAddress(uint32_t address, uint32_t* offset, uint32_t* setnum, uint32_t* tag) {
		uint32_t mask;

		mask = PTWO(offsetSize) - 1;
		*offset = mask & address;

		mask = (PTWO(setSize) - 1) * PTWO(offsetSize);
		*setnum = (mask & address) >> offsetSize;

		mask = (PTWO(tagSize) - 1) * PTWO(offsetSize + setSize);
		*tag = (mask & address) >> (offsetSize + setSize);
	}



	void write(uint32_t address) {
		uint32_t offset, setnum, tag;
		getFromAddress(address, &offset, &setnum, &tag);

		DEBUG << address << " " << offset << " " << setnum << " " << tag << endl;

		int way = search(address);
		bool hit = way != -1;

		accessCounter++;
		timeAccu += cycles;

		if (hit) {
			DEBUG << "write hit" << endl;
			updateLru(setnum, way);
		}
		else {
			DEBUG << "write miss" << endl;
			missCounter++;
			if (below != nullptr)
				below->write(address);
			else
				timeAccu += memCycles;

			// add to self according to lru
			if (writeAlloc) {
				int x = getLruMin(setnum);

				// check if x is dirty
				if (ways[x][setnum].dirty == 1) {
					if (below != nullptr) {
						int y; uint32_t ySet;
						getWayLruByAddress(ways[x][setnum].address, &y, &ySet);
						below->ways[y][ySet].dirty = 1;
						below->updateLru(ySet, y);
					}
				}

				// check if we remove something:
				if (ways[x][setnum].valid == 1) {
					// check for lower level for the inclusion thingy
					if (above != nullptr) {
						int wayAbove = above->search(ways[x][setnum].address);
						if (wayAbove != -1) {
							uint32_t offsetAbove, setnumAbove, tagAbove;
							above->getFromAddress(ways[x][setnum].address, &offsetAbove, &setnumAbove, &tagAbove);
							above->ways[wayAbove][setnumAbove].valid = 0;
						}
					}
				}

				
				ways[x][setnum].tag = tag;
				ways[x][setnum].valid = 1;
				ways[x][setnum].dirty = 1;
				ways[x][setnum].address = address;
				updateLru(setnum, x);
			}
		}

	}

	void read(uint32_t address) {
		uint32_t offset, setnum, tag;
		getFromAddress(address, &offset, &setnum, &tag);

		int way = search(address);
		bool hit = way != -1;

		accessCounter += 1;
		timeAccu += cycles;

		if (hit) {
			updateLru(setnum, way);
		}
		else {
			missCounter++;
			if (below != nullptr)
				below->read(address);
			else
				timeAccu += memCycles;

			
			int x = getLruMin(setnum);

			// check if x is dirty
			if (ways[x][setnum].dirty == 1) {
				if (below != nullptr) {
					int y; uint32_t ySet;
					getWayLruByAddress(ways[x][setnum].address, &y, &ySet);
					below->ways[y][ySet].dirty = 1;
					below->updateLru(ySet, y);
				}
			}

			// check if we remove something:
			if (ways[x][setnum].valid == 1) {
				// check for lower level for the inclusion thingy
				if (above != nullptr) {
					int wayAbove = above->search(ways[x][setnum].address);
					if (wayAbove != -1) {
						uint32_t offsetAbove, setnumAbove, tagAbove;
						above->getFromAddress(ways[x][setnum].address, &offsetAbove, &setnumAbove, &tagAbove);
						above->ways[wayAbove][setnumAbove].valid = 0;
					}
				}
			}

			ways[x][setnum].tag = tag;
			ways[x][setnum].valid = 1;
			ways[x][setnum].address = address;
			updateLru(setnum, x);
			
		}

	}

	
};

int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}
	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	// setup
	cache L1(L1Size, L1Assoc, L1Cyc, BSize, MemCyc, WrAlloc);
	cache L2(L2Size, L2Assoc, L2Cyc, BSize, MemCyc, WrAlloc);
	
	L1.below = &L2;
	L2.above = &L1;

	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		//cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		//cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		//cout << " (dec) " << num << endl;
		static int i = 0;
		//if (i == 9) debug = true; else debug = false;

		if (operation == 'w') {
			L1.write(num);
			//cout << i << ": " << L1.missCounter << " " << L1.accessCounter << ", " << L2.missCounter << " " << L2.accessCounter << ", " << L1.timeAccu << " " << L2.timeAccu << endl;
		}

		if (operation == 'r') {
			L1.read(num);
			//cout << i << ": " << L1.missCounter << " " << L1.accessCounter << ", " << L2.missCounter << " " << L2.accessCounter << ", " << L1.timeAccu << " " << L2.timeAccu << endl;
		}
		i += 1;

	}

	double L1MissRate = 0;
	double L2MissRate = 0;
	double avgAccTime = 0;


	L1MissRate = (double)L1.missCounter / (double)L1.accessCounter;
	L2MissRate = (double)L2.missCounter / (double)L2.accessCounter;
	avgAccTime = ((double)L1.timeAccu + (double)L2.timeAccu) / (double)L1.accessCounter;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
