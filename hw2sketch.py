import argparse
from random import randint, choice
from math import log2

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
	
class CacheEntry:
	def __init__(self):
		self.tag = 0
		self.valid = 0
		self.dirty = 0
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
		
		self.ways = [None] * 2**self.assoc
		for way in range(2**self.assoc):
			self.ways[way] = []
			for i in range(2**self.size):
				self.ways[way].append(CacheEntry())
		
		self.name = name
	def __str__(self):
		string = '--- Cache: ' + self.name + ' size: ' + str(self.size) + " assoc: " + str(self.assoc) + " cycles: " + str(self.cycles) + " ---\n"
		for s in range(2**self.size):
			for w in range(2**self.assoc):
				string += str(self.ways[w][s]) + " "
			string += "\n"
		return string

args = parseArgs()

file = open(args.file, 'r')
for line in file:
	action, address = line.split()

l1 = Cache(3,2,2, "L1")
l2 = Cache(3,3,3, "L2")
l2._next = l1
l1.ways[2][5].tag = 0xc0dec0de
print(l1)




file.close()


























