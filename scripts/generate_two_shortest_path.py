#!/usr/bin/python
#
# Generates a graph where using priorities will be 2x faster, on average.

import sys

if len(sys.argv) < 2:
	print "usage: python generate_two_shortest_path.py <number of extra nodes>"
	sys.exit(1)

total = int(sys.argv[1])

print "type route edge(node, node, int)."
print ""
print "!edge(@1, @2, 1)."
print "!edge(@2, @4, 1)."
print "!edge(@3, @2, 1)."
print "!edge(@3, @4, 6)."
print ""

for x in range(5, total + 5):
	print "!edge(@1, @" + str(x) + ", 5)."
	print "!edge(@" + str(x) + ", @3, 5)."
