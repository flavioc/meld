#!/usr/bin/python
#
# This transform adj_lists from the webpage
# http://www.cs.toronto.edu/~tsap/experiments/datasets/index.html
# into Meld source code files.
#

import sys
from lib import simple_write_edge

if len(sys.argv) != 2:
	print "usage: transform_link_analysis_undirected.py <file>"
	sys.exit(1)

file = sys.argv[1]
f = open(file, 'rb')

print "type route edge(node, node)."
print

edge_list = {}

def try_write(a, b):
	try:
		true = edge_list[(a, b)]
	except KeyError:
		edge_list[(a, b)] = True
		simple_write_edge(a, b)

for line in f:
	line = line.rstrip('\n')
	vec = line.split(' ')
	id = int(vec[0].rstrip(':'))
	size = len(vec)-2
	if size > 0:
		w = 1.0 / float(size)
		for dest in vec[1:len(vec)-1]:
			a = id
			b = int(dest)
			try_write(a, b)
			try_write(b, a)

