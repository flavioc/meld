#!/usr/bin/python

import sys
import random
from lib import *

print_inout = True
OUTPUT_MELD = True

if len(sys.argv) < 2:
	print "Usage: generate_random_graph.py <num nodes> [weight]"
	sys.exit(1)

total = int(sys.argv[1])

if OUTPUT_MELD:
	if print_inout:
		print "type route input(node, node, float)."
		print "type route output(node, node, float)."	
	else:
		if len(sys.argv) > 2:
			set_weight(float(sys.argv[2]))
			print "type route edge(node, node, float)."
		else:
			print "type route edge(node, node)."
else:
	print total

if not OUTPUT_MELD:
	print_inout = True

nodes = generate_graph(total, True)
for i, list in nodes.iteritems():
	if print_inout:
		count = float(len(list))
		w = 1.0 / count
		if not OUTPUT_MELD:
			print str(i) + ":",
		for link in list:
			if OUTPUT_MELD:
				write_winout(link, i, w)
			else:
				print link,
		if not OUTPUT_MELD:
			print
	else:
		for link in list:
			write_edge(i, link)
		
