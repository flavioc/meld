#!/usr/bin/python

import sys

from lib import write_edge
from lib import set_weight

if len(sys.argv) < 2:
	print "Usage: generate_chain.py <num nodes> [weight]"
	sys.exit(1)

print "type route edge(node, node)."

if len(sys.argv) == 3:
	set_weight(int(sys.argv[2]))

previous = 0
for i in range(1, int(sys.argv[1])):
	new = i
	write_edge(previous, new)
	previous = new

