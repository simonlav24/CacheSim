/* 046267 Computer Architecture - Spring 2021 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>

#define ADDRESS_SIZE 32

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

// cahce entry - a cache line
struct cacheEntry {
	int tag;
	int valid;
	int dirty;
};

// cache class
class cache {
	// parameters:
	int size;
	int assoc;
	int cycles;
	// cache level pointers 
	cache* below;
	cache* above;
	// counters 
	int timeAccu;
	int missCounter;
	int accessCounter;
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
	cache(int size, int assoc, int cycles, unsigned blockSize, unsigned memCycles, unsigned writeAlloc){
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
		this->ways = new cacheEntry * [pow(2, this->assoc)];
		for (int way = 0; way < pow(2, this->assoc); way++) {
			this->ways[way] = new cacheEntry[pow(2, this->setSize)];
		}

		// allocating and initializing lru array
		this->lru = new int*[pow(2, this->setSize)];
		for (int i = 0; i < pow(2, this->setSize); i++) {
			lru[i] = new int[pow(2, this->assoc)];
			for (int j = 0; j < pow(2, this->assoc); j++) {
				lru[i][j] = j;
			}
		}
	}
	~cache() {
		// deallocating ways array
		for (int way = 0; way < pow(2, this->assoc); way++) {
			delete[] this->ways[way];
		}
		delete[] ways;

		// deallocating lru array
		for (int i = 0; i < pow(2, this->setSize); i++) {
			delete[] lru[i];
		}
		delete[] lru;
	}

	void updateLru(int setnum, int way) {
		int x = lru[setnum][way];
		lru[setnum][way] = pow(2, assoc) - 1;
		for (int j = 0; j < pow(2, assoc); j++) {
			if (j != way && lru[setnum][j] > x) {
				lru[setnum][j] -= 1;
			}
		}
	}
	
	int getLruMin(int setnum) {
		for (int i = 0; i < pow(2, assoc); i++)
			if (lru[setnum][i] == 0)
				return i;
		return -1;
	}

	int search(uint32_t address) {
		uint32_t offset, setnum, tag;
		getFromAddress(address, &offset, &setnum, &tag);

		for (int way = 0; way < pow(2, assoc); way++)
			if (ways[way][setnum].tag == tag && ways[way][setnum].valid == 1)
				return way;
		return -1;
	}

	void getFromAddress(uint32_t address, uint32_t* offset, uint32_t* setnum, uint32_t* tag) {
		uint32_t mask;

		mask = pow(2, offsetSize) - 1;
		*offset = mask & address; // was address in py

		mask = (pow(2, setSize) - 1) * pow(2, offsetSize);
		*setnum = (mask & address) >> offsetSize;

		mask = (pow(2, tagSize) - 1) * pow(2, offsetSize + setSize);
		*tag = (mask & address) >> (offsetSize + setSize);
	}

	void write(uint32_t address) {
		uint32_t offset, setnum, tag;
		getFromAddress(address, &offset, &setnum, &tag);

		int way = search(address);
		bool hit = way != -1;

		accessCounter++;
		timeAccu += cycles;

		if (hit) {
			updateLru(setnum, way);
		}
		else {
			missCounter++;
			if (below != nullptr)
				below->write(address);
			else
				timeAccu += memCycles;

			if (writeAlloc) {
				int x = getLruMin(setnum);
				ways[x][setnum].tag = tag;
				ways[x][setnum].valid = 1;
				ways[x][setnum].dirty = 1;
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

			if (writeAlloc) {
				int x = getLruMin(setnum);
				ways[x][setnum].tag = tag;
				ways[x][setnum].valid = 1;
				updateLru(setnum, x);
			}
		}

	}
	/*		
		
		if hit:
			self.updateLru(setnum, way)
			# print(" - read hit")
			
		else:
			# print(" - read miss")
			self.missCounter += 1
			if self.below:
				self.below.read(address)
			else:
				self.timeAccu += args.mem_cyc
			
			# add to self according to lru
			x = self.getLruMin(setnum)
			self.ways[x][setnum].tag = tag
			self.ways[x][setnum].valid = 1
			self.updateLru(setnum, x)*/

};

int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}
	cout << "hello" << endl;
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
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;

	}

	double L1MissRate = 0;
	double L2MissRate = 0;
	double avgAccTime = 0;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
