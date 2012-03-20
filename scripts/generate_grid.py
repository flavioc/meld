#!/usr/bin/python

import sys

from lib import write_edge
from lib import set_weight

if len(sys.argv) < 2:
	print "Usage: generate_grid.py <num nodes> [weight]"
	sys.exit(1)

if len(sys.argv) == 3:
	set_weight(int(sys.argv[2]))
	print "type route edge(node, node, int)."
else:
	print "type route edge(node, node)."

arg = sys.argv[1]
lenvec = arg.split('@', 2)

if len(lenvec) == 2:
   width = int(lenvec[0])
   height = int(lenvec[1])
else:
   width = int(lenvec[0])
   height = width

def write_line(line):
	for column in range(0, width):
		id = line * width + column
		if line < height-1:
			southid = id + width
			write_edge(southid, id)
			write_edge(id, southid)
		if column < width-1:
			eastid = id + 1
			write_edge(eastid, id)
			write_edge(id, eastid)
			
for line in range(0, height):
	write_line(line)
