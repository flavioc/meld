#!/usr/bin/python
# Uniformize ID's in a SNAP STANFORD file.

import sys

if len(sys.argv) != 2:
	print "usage: uniform_snap.py <file>"
  	sys.exit(1)

ids = {}
count = 0
for line in open(sys.argv[1], "r"):
	line = line.rstrip().lstrip()
	if line.startswith('#'):
		continue
	vec = line.split('\t')
	if len(vec) == 1:
		vec = line.split(' ')
	if len(vec) != 2:
		continue
	id1 = int(vec[0])
	id2 = int(vec[1])
	try:
		realid1 = ids[id1]
	except KeyError:
		realid1 = count
		ids[id1] = realid1
		count = count + 1
	try:
		realid2 = ids[id2]
	except KeyError:
		realid2 = count
		ids[id2] = realid2
		count = count + 1
	print str(realid1) + "\t" + str(realid2)
