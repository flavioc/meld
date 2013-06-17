#!/usr/bin/python

import sys
import random
from lib import *

def list_has(list, x):
	try:
		list.index(x)
		return True
	except ValueError:
		return False

if len(sys.argv) < 2:
	print "Usage: generate_random_undirected_graph.py <num nodes> [weight]"
	sys.exit(1)

total = int(sys.argv[1])

if len(sys.argv) > 2:
	set_weight(float(sys.argv[2]))
	print "type route edge(node, node, float)."
else:
	print "type route edge(node, node)."

for i in range(total):
	if i == total-1:
		continue
	links = random.randint(1, int((total - i)*0.75))
	list = []
	for j in range(links):
		link = random.randint(i + 1, total - 1)
		if not list_has(list, link):
			list.append(link)
	for link in list:
		write_dedge(i, link)
		
