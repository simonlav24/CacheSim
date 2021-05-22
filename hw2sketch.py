import argparse
from random import randint, choice
from math import log2

ADDRESS_SIZE = 32

def parseArgs():
	"""
	./cacheSim <input file> --mem-cyc <# of cycles> --bsize <block log2(size)>
	--wr-alloc <0: No Write Allocate; 1: Write Allocate>
	--l1-size <log2(size)> --l1-assoc <log2(# of ways)> --l1-cyc <# of cycles>
	--l2-size <log2(size)> --l2-assoc <log2(# of ways)> --l2-cyc <# of cycles>
	"""

	parser = argparse.ArgumentParser()
	
	parser.add_argument('file', type=str)
	
	parser.add_argument('--mem-cyc', type=int)
	parser.add_argument('--bsize', type=int)
	parser.add_argument('--wr-alloc', type=int)
	
	parser.add_argument('--l1-size', type=int)
	parser.add_argument('--l1-assoc', type=int)
	parser.add_argument('--l1-cyc', type=int)
	
	parser.add_argument('--l2-size', type=int)
	parser.add_argument('--l2-assoc', type=int)
	parser.add_argument('--l2-cyc', type=int)
	
	return parser.parse_args()

def binary(x):
	return format(x, "032b")

debug = False 

class CacheEntry:
	def __init__(self):
		self.tag = 0
		self.valid = 0
		self.dirty = 0
		self.address = 0
	def __str__(self):
		return "[ " + str(self.valid) + " | " + str(self.dirty) + " | " + '{:10}'.format(hex(self.tag)) + " ]"
	def __repr__(self):
		return str(self)
	
class Cache:
	def __init__(self, size, assoc, cycles, name="L"):
		self.size = size
		self.assoc = assoc
		self.cycles = cycles
		
		self.below = None
		self.above = None
		
		self.timeAccu = 0
		self.missCounter = 0
		self.accessCounter = 0
		
		self.offsetSize = args.bsize
		self.setSize = self.size - self.offsetSize - self.assoc
		self.tagSize = ADDRESS_SIZE - self.setSize - self.offsetSize
	
		
		self.ways = [None] * 2**self.assoc
		for way in range(2**self.assoc):
			self.ways[way] = []
			for i in range(2**self.setSize):
				self.ways[way].append(CacheEntry())
				
		self.lru = [None] * 2**self.setSize
		for l in range(2**self.setSize):
			self.lru[l] = [i for i in range(2**self.assoc)]
		
		self.name = name
	
	def updateLru(self, setnum, way):
		# print(self.lru)
		x = self.lru[setnum][way]
		self.lru[setnum][way] = 2 ** self.assoc - 1
		for j in range(2 ** self.assoc):
			if(j != way and self.lru[setnum][j] > x):
				self.lru[setnum][j] -= 1
		
	def getLruMin(self, setnum):
		index = self.lru[setnum].index(0)
		return index
	
	def search(self, address):
		offset, setnum, tag = self.getFromAddress(address)
		
		for way in range(2 ** self.assoc):
			if self.ways[way][setnum].tag == tag and self.ways[way][setnum].valid == 1:
				return (True, way)
		return (False, -1)
	
	def getFromAddress(self, address):
		offsetSize = args.bsize
		setSize = self.size - offsetSize - self.assoc
		tagSize = ADDRESS_SIZE - setSize - offsetSize
		if debug:print("offsetSize:", offsetSize, "setSize:", setSize, "tagSize:", tagSize)
		
		
		mask = 2**(offsetSize) - 1
		offset = mask & address
		
		mask = (2**(setSize) - 1) * 2**offsetSize
		setnum = (mask & address) >> offsetSize
		
		mask = (2**(tagSize) - 1) * 2**(offsetSize + setSize)
		tag = (mask & address) >> (offsetSize + setSize)
		
		if debug:print("cache:", self.name, "address:", address, "tag:", hex(tag), "setnum:", setnum, "offset:", offset)
		
		return (offset, setnum, tag)
	
	# def buildAddress(self, offset, setnum, tag):
		# address = tag
		# address = address << (self.setSize)
		# address += setnum
		# address = address << (self.offsetSize)
		# address += offset
		# print(hex(address))
		# return address
	
	def getWayLruByAddress(self, address):
		offset, setnum, tag = self.getFromAddress(address)
		for way in range(2**self.assoc):
			if self.ways[way][setnum].tag == tag:
				if debug:print("wayyyyy:", way)
				return (way, setnum)
		return (-1, -1)
	
	def write(self, address):
		# print("writing", self.name)
		offset, setnum, tag = self.getFromAddress(address)
		if debug:print(offset, setnum, hex(tag))
		hit, way = self.search(address)
		
		
		self.accessCounter += 1
		self.timeAccu += self.cycles
		
		if hit:
			self.updateLru(setnum, way)
			
		else:
			self.missCounter += 1
			if self.below:
				self.below.write(address)
			else:
				self.timeAccu += args.mem_cyc
			
			# add to self according to lru
			if args.wr_alloc == 1: # write allocate (need to bring into cache)
				x = self.getLruMin(setnum)
				# check if x is dirty:
				if self.ways[x][setnum].dirty == 1:
					if(self.below):
						y, ySet = self.below.getWayLruByAddress(self.ways[x][setnum].address)
						if debug:print("way of thing to update:", y, "set of the thing to update:", ySet)
						self.below.ways[y][ySet].dirty = 1
						self.below.updateLru(ySet, y)
				
				# check if we remove something:
				if self.ways[x][setnum].valid == 1:
					# check for lower level for the inclusion thingy
					if self.above:
						hitAbove, wayAbove = self.above.search(self.ways[x][setnum].address)
						if hitAbove:
							# remove by invalidating:
							offsetAbove, setnumAbove, tagAbove = self.above.getFromAddress(self.ways[x][setnum].address)
							self.above.ways[wayAbove][setnumAbove].valid = 0
				
				self.ways[x][setnum].tag = tag
				self.ways[x][setnum].valid = 1
				self.ways[x][setnum].dirty = 1
				self.ways[x][setnum].address = address
				self.updateLru(setnum, x)
			
			
	def read(self, address):
		# print("reading", self.name)
		offset, setnum, tag = self.getFromAddress(address)
		if debug:print(offset, setnum, hex(tag))
		hit, way = self.search(address)
		
		self.accessCounter += 1
		self.timeAccu += self.cycles
		
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
			if self.ways[x][setnum].dirty == 1:
					if(self.below):
						y, ySet = self.below.getWayLruByAddress(self.ways[x][setnum].address)
						if debug:print("way of thing to update:", y, "set of the thing to update:", ySet)
						self.below.ways[y][ySet].dirty = 1
						self.below.updateLru(ySet, y)
			
			# check if we remove something:
			if self.ways[x][setnum].valid == 1:
				# check for lower level for the inclusion thingy
				if self.above:
					hitAbove, wayAbove = self.above.search(self.ways[x][setnum].address)
					if hitAbove:
						# remove by invalidating:
						offsetAbove, setnumAbove, tagAbove = self.above.getFromAddress(self.ways[x][setnum].address)
						self.above.ways[wayAbove][setnumAbove].valid = 0
			
			self.ways[x][setnum].tag = tag
			self.ways[x][setnum].valid = 1
			self.ways[x][setnum].address = address
			self.updateLru(setnum, x)
			
	
	
	def __str__(self):
		string = '--- Cache: ' + self.name + ' size: ' + str(self.size) + " assoc: " + str(self.assoc) + " cycles: " + str(self.cycles) + " ---\n"
		for setnum in range(2**self.setSize):
			for way in range(2**self.assoc):
				string += str(self.ways[way][setnum].valid) + " | " + str(self.ways[way][setnum].dirty) + " | " + hex(self.ways[way][setnum].tag)
				string += "    "
			string += "\n"
		
		# string += str(self.ways) + "\n"
		string += str(self.lru) + "\n"
		return string
	def printValid(self):
		string = '--- Cache: ' + self.name + ' size: ' + str(self.size) + " assoc: " + str(self.assoc) + " cycles: " + str(self.cycles) + " ---\n"
		for s in range(2**self.size):
			printed = False
			for w in range(2**self.assoc):
				if self.ways[w][s].valid == 1:
					string += "(" + str(w) + ", " + str(s) + ")" + str(self.ways[w][s]) + " "
					printed = True
			if printed:
				string += "\n"
		return string

args = parseArgs()

### MAIN

l1 = Cache(args.l1_size ,args.l1_assoc, args.l1_cyc, "L1")
l2 = Cache(args.l2_size ,args.l2_assoc, args.l2_cyc, "L2")

l1.below = l2
l2.above = l1


# offsetT, setnumT, tagT = l1.getFromAddress(0x12345678)
# l1.buildAddress(offsetT, setnumT, tagT)
# print(1/0)


i = 0
file = open(args.file, 'r')
for line in file:
	action, address = line.split()
	
	# get address
	address = int(address, 0)

	if debug:print("address:", hex(address))
	if action == 'w':
		# print("--- writing ---")
		l1.write(address)
		if debug:print(l1)
		if debug:print(l2)
		#print(i, "r", ":", l1.missCounter, l1.accessCounter, l2.missCounter, l2.accessCounter, l1.timeAccu, l2.timeAccu)
		
	
	if action == 'r':
		# print("--- reading ---")
		l1.read(address)
		if debug:print(l1)
		if debug:print(l2)
		#rint(i, "r", ":", l1.missCounter, l1.accessCounter, l2.missCounter, l2.accessCounter, l1.timeAccu, l2.timeAccu)
	i += 1
	
if debug:print(l1.missCounter, l1.accessCounter, l2.missCounter, l2.accessCounter, l1.timeAccu, l2.timeAccu)
	
# print(l1.accessCounter, l2.accessCounter)
l1missrate = l1.missCounter / l1.accessCounter
l2missrate = l2.missCounter / l2.accessCounter
accTimeAvg = (l1.timeAccu + l2.timeAccu) / l1.accessCounter
print("L1miss=" + str(l1missrate) + " " + "L2miss=" + str(l2missrate) + " " + "AccTimeAvg=" + str(accTimeAvg))
	


file.close()


























