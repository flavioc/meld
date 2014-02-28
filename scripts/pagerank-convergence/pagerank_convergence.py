#!/usr/bin/python

import sys

if len(sys.argv) != 4:
	print "need more arguments"
	sys.exit(1)

file1 = sys.argv[1]
file2 = sys.argv[2]
delta = float(sys.argv[3])

with open(file1) as f1, open(file2) as f2:
	for x, y in zip(f1, f2):
		x = x.strip()
		y = y.strip()
		v1 = float(x)
		v2 = float(y)
		if abs(v1 - v2) > delta:
			sys.exit(1)

sys.exit(0)
