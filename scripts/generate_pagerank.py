#!/usr/bin/python

import sys
import random

def list_has(list, x):
	try:
		list.index(x)
		return True
	except ValueError:
		return False

if len(sys.argv) < 2:
	print "Usage: generate_pagerank.py <num nodes> [weight]"
	sys.exit(1)

total = int(sys.argv[1])
weight = None

if len(sys.argv) > 2:
   weight = float(sys.argv[2])

print "type route edge(node, node)."
if weight is not None:
	print "type weight(node, node, float)."

for i in range(total):
	links = random.randint(1, int(total*0.75))
	list = []
	for j in range(links):
		link = random.randint(1, total - 1)
		if link == i:
			link = (link + 1) % (total - 1)
		if not list_has(list, link):
			list.append(link)
	for link in list:
		print "!edge(@" + str(i) + ",@" + str(link) + ")."
		if weight is not None:
			print "!weight(@" + str(i) + ",@" + str(link) + "," + str(weight) + ")."
		
