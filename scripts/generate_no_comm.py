#!/usr/bin/python

import sys

if len(sys.argv) != 2:
	print "Usage: generate_no_comm.py <num nodes>"
	sys.exit(1)

total = int(sys.argv[1])
for i in range(total):
	print "counter(@" + str(i) + ", 0)."
