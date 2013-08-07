#!/usr/bin/python
#
# Converts tnet files into an undirected graph structure.
#
# http://toreopsahl.com/datasets/
#

import sys
from lib import write_edge
from lib import set_weight

if len(sys.argv) < 2:
   print "Usage: tnet.py <file>"
   sys.exit(1)
   
f = open(sys.argv[1], "rb")

print "type route edge(node, node)."

edges = {}

def try_write(a, b):
	try:
		true = edges[(a, b)]
	except KeyError:
		edges[(a, b)] = True
		write_edge(a, b)

for line in f:
	vec = line.rstrip().split(' ')
	a = int(vec[0])
	b = int(vec[1])
	try_write(a, b)
	try_write(b, a)
