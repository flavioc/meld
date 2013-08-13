#!/usr/bin/python
#
# This transform adj_lists from the webpage
# http://www.cs.toronto.edu/~tsap/experiments/datasets/index.html
# into Meld source code files.
#

import sys
from lib import write_edgew

if len(sys.argv) != 2:
	print "usage: transform_link_analysis_shortest_path.py <file>"
	sys.exit(1)

file = sys.argv[1]
f = open(file, 'rb')

print "type edge(node, node, int)."
print

for line in f:
	line = line.rstrip('\n')
	vec = line.split(' ')
	id = vec[0].rstrip(':')
	for dest in vec[1:len(vec)-1]:
		write_edgew(id, dest, 1)

