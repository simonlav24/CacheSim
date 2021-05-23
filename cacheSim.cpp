/* 046267 Computer Architecture - Spring 2021 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>

#define ADDRESS_SIZE 32
#define PTWO(x) (1 << (x))
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
	unsigned int tag;
	int valid;
	int dirty;
	unsigned long int address;
};

// debug print entry
void printEntry(cacheEntry x) {
	cout << x.valid << " | " << x.dirty << " | " << x.tag << endl;
}

// cache class
class cache {
	// parameters:
	unsigned int size;
	unsigned int assoc;
	unsigned int cycles;
	
	// field sizes
	unsigned int offsetSize;
	unsigned int memCycles;
	bool writeAlloc;
	unsigned int setSize;
	unsigned int tagSize;
	// ways 2darray of cache entries
	cacheEntry** ways;
	// lru array 
	int** lru;
	
public:

	// counters 
	unsigned int timeAccu;
	unsigned int missCounter;
	unsigned int accessCounter;

	// cache level pointers 
	cache* below;
	cache* above;

    // constructor
	cache(unsigned int size, unsigned int assoc, unsigned int cycles, unsigned int blockSize, unsigned int memCycles, unsigned int writeAlloc){
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
        
        below = NULL;
        above = NULL;
	}
    // distructor
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

    // get way and set of address to update LRU
	void getWayLruByAddress(unsigned long int address, int* _way, unsigned int* _setnum) {
		unsigned int offset=0, setnum=0, tag=0;
		getFromAddress(address, &offset, &setnum, &tag);
		for (int way = 0; way < PTWO(assoc); way++) {
			if (ways[way][setnum].tag == tag) {
				*_way = way;
				*_setnum = setnum;
			}
		}
	}

    // update LRU based on set and way
	void updateLru(unsigned int setnum, int way) {
		int x = lru[setnum][way];
		lru[setnum][way] = PTWO(assoc) - 1;
		for (int j = 0; j < PTWO(assoc); j++) {
			if (j != way && lru[setnum][j] > x) {
				lru[setnum][j] -= 1;
			}
		}
	}
	
    // get the minimum LRU value (oldest accessed)
	int getLruMin(unsigned int setnum) {
		for (int i = 0; i < PTWO(assoc); i++)
			if (lru[setnum][i] == 0)
				return i;
		return -1;
	}

    // search cache by address
	int search(unsigned long int address) {
		unsigned int offset=0, setnum=0, tag=0;
		getFromAddress(address, &offset, &setnum, &tag);

		for (int way = 0; way < PTWO(assoc); way++) {
			if (ways[way][setnum].tag == tag && ways[way][setnum].valid == 1) {
				return way;
			}
		}
		return -1;
	}

    // parse address to get offset, set and tag
	void getFromAddress(unsigned long int address, unsigned int* offset, unsigned int* setnum, unsigned int* tag) {
		unsigned long int mask=PTWO(offsetSize) - 1;

		mask = PTWO(offsetSize) - 1;
		*offset = mask & address;

		mask = (PTWO(setSize) - 1) * PTWO(offsetSize);
		*setnum = (mask & address) >> offsetSize;

		mask = (PTWO(tagSize) - 1) * PTWO(offsetSize + setSize);
		*tag = (mask & address) >> (offsetSize + setSize);
	}

    // write Function
	void write(unsigned long int address) {
        // parse address
		unsigned int offset=0, setnum=0, tag=0;
		getFromAddress(address, &offset, &setnum, &tag);

		DEBUG << address << " " << offset << " " << setnum << " " << tag << endl;

		int way = search(address);
		bool hit = way != -1;
        // update counters
		accessCounter++;
		timeAccu += cycles;

        // maintain cache and LRU
		if (hit) {
			DEBUG << "write hit" << endl;
			updateLru(setnum, way);
		}
		else {
			DEBUG << "write miss" << endl;
			missCounter++;
			if (below != NULL)
				below->write(address);
			else
				timeAccu += memCycles;

			// add to self according to lru
			if (writeAlloc) {
				int x = getLruMin(setnum);

				// check if x is dirty
				if (ways[x][setnum].dirty == 1) {
					if (below != NULL) {
						int y=0; unsigned int ySet=0;
						getWayLruByAddress(ways[x][setnum].address, &y, &ySet);
						below->ways[y][ySet].dirty = 1;
						below->updateLru(ySet, y);
					}
				}

				// check if we remove something:
				if (ways[x][setnum].valid == 1) {
					// check for lower level for the inclusion thingy
					if (above != NULL) {
						int wayAbove = above->search(ways[x][setnum].address);
						if (wayAbove != -1) {
							unsigned int offsetAbove=0, setnumAbove=0, tagAbove=0;
							above->getFromAddress(ways[x][setnum].address, &offsetAbove, &setnumAbove, &tagAbove);
							above->ways[wayAbove][setnumAbove].valid = 0;
						}
					}
				}
				// add to cache
				ways[x][setnum].tag = tag;
				ways[x][setnum].valid = 1;
				ways[x][setnum].dirty = 1;
				ways[x][setnum].address = address;
				updateLru(setnum, x);
			}
		}
	}

	void read(unsigned long int address) {
		// parse address
        unsigned int offset=0, setnum=0, tag=0;
		getFromAddress(address, &offset, &setnum, &tag);
        
		int way = search(address);
		bool hit = way != -1;
        // update counters
		accessCounter += 1;
		timeAccu += cycles;

        // maintain cache and LRU
		if (hit) {
			updateLru(setnum, way);
		}
		else {
			missCounter++;
			if (below != NULL)
				below->read(address);
			else
				timeAccu += memCycles;
			
			int x = getLruMin(setnum);

			// check if x is dirty
			if (ways[x][setnum].dirty == 1) {
				if (below != NULL) {
					int y=0; unsigned int ySet=0;
					getWayLruByAddress(ways[x][setnum].address, &y, &ySet);
					below->ways[y][ySet].dirty = 1;
					below->updateLru(ySet, y);
				}
			}

			// check if we remove something:
			if (ways[x][setnum].valid == 1) {
				// check for lower level for the inclusion thingy
				if (above != NULL) {
					int wayAbove = above->search(ways[x][setnum].address);
					if (wayAbove != -1) {
						unsigned int offsetAbove=0, setnumAbove=0, tagAbove=0;
						above->getFromAddress(ways[x][setnum].address, &offsetAbove, &setnumAbove, &tagAbove);
						above->ways[wayAbove][setnumAbove].valid = 0;
					}
				}
			}
            // add to cache
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

	unsigned int MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
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

		string cutAddress = address.substr(2); // Removing the "0x" part of the address
		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		if (operation == 'w') {
			L1.write(num);
		}

		if (operation == 'r') {
			L1.read(num);
		}

	}

	double L1MissRate = 0;
	double L2MissRate = 0;
	double avgAccTime = 0;

    // calculate results data
	L1MissRate = (double)L1.missCounter / (double)L1.accessCounter;
	L2MissRate = (double)L2.missCounter / (double)L2.accessCounter;
	avgAccTime = ((double)L1.timeAccu + (double)L2.timeAccu) / (double)L1.accessCounter;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
