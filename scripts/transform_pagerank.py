#!/usr/bin/python
#
# This transform adj_lists from the webpage
# http://www.cs.toronto.edu/~tsap/experiments/datasets/index.html
# into Meld source code files.
#

import sys
from lib import simple_write_edge

if len(sys.argv) != 2:
	print "usage: transform_pagerank.py <file>"
	sys.exit(1)

output_meld = True
file = sys.argv[1]
f = open(file, 'rb')

if output_meld:
	print "type input(node, node, float)."
	print "type output(node, node, float)."
	print

def write_inout(i, o, w):
	if output_meld:
		print "!output(@" + str(i) + ", @" + str(o) + ", " + str(w) + ")."
		print "!input(@" + str(o) + ", @" + str(i) + ", " + str(w) + ")."
	else:
		print str(i) + "\t" + str(o) + "\t" + str(w)

edge_list = {}

for line in f:
	line = line.rstrip('\n')
	vec = line.split(' ')
	id = vec[0].rstrip(':')
	size = len(vec)-2
	if size > 0:
		w = 1.0 / float(size)
		for dest in vec[1:len(vec)-1]:
			write_inout(id, dest, w)
