#!/usr/bin/python

import sys
import random
from lib import *

print_inout = True
OUTPUT_MELD = False

def list_has(list, x):
	try:
		list.index(x)
		return True
	except ValueError:
		return False

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

def get_factor():
	if total < 200:
		return 0.5
	else:
		return 1.0/(total/200.0)

for i in range(total):
	links = random.randint(1, int(total*get_factor()))
	list = []
	for j in range(links):
		link = random.randint(1, total - 1)
		if link == i:
			link = (link + 1) % total
		if not list_has(list, link):
			list.append(link)
	if print_inout:
		count = float(len(list))
		w = 1.0 / count
		if not OUTPUT_MELD:
			print str(i) + ":",
		for link in list:
			if OUTPUT_MELD:
				write_winout(i, link, w)
			else:
				print link,
		if not OUTPUT_MELD:
			print

	else:
		for link in list:
			write_edge(i, link)
		
