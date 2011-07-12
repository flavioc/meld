#!/usr/bin/python

import sys

from lib import write_edge
from lib import set_weight

if len(sys.argv) < 2:
	print "Usage: generate_grid.py <num nodes> [weight]"
	sys.exit(1)

print "type route edge(node, node)."

if len(sys.argv) == 3:
	set_weight(int(sys.argv[2]))

length = int(sys.argv[1])

def write_line(line):
	for column in range(0, length):
		id = line * length + column
		if line < length-1:
			southid = id + length
			write_edge(southid, id)
			write_edge(id, southid)
		if column < length-1:
			eastid = id + 1
			write_edge(eastid, id)
			write_edge(id, eastid)
			
for line in range(0, length):
	write_line(line)
