#!/usr/bin/python

import sys
sys.setrecursionlimit(50000)

from lib import write_edge
from lib import set_weight

if len(sys.argv) < 2:
	print "Usage: generate_pyramid.py <num nodes> [weight]"
	sys.exit(1)


if len(sys.argv) == 3:
	set_weight(int(sys.argv[2]))
	print "type route edge(node, node, int)."
else:
	print "type route edge(node, node)."

def write_pyramid(depth, z):
	if depth == 1:
		write_edge(z, z+1)
	else:
		write_edge(z, z+1)
		write_edge(z, z+2)
		write_edge(z+1, z+3)
		write_pyramid(depth - 1, z + 2)

depth = int(sys.argv[1])
write_pyramid(depth, 0)
