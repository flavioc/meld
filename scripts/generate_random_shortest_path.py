#!/usr/bin/python
#
# Generates a random graph for use with the random shortest path program.

import sys
import random

if len(sys.argv) < 2:
	print "usage: python generate_random_shortest_path.py <size nodes>"
	sys.exit(1)

total = int(sys.argv[1])
SIZE_PATH = int(float(total) / 2.0)

links = []

def add_link(src, dst):
	if not (src, dst) in links:
		links.append((src, dst))
		
nodes = [1, 2, 3, 4]
add_link(1, 2)
add_link(2, 3)
last = 3

while len(nodes) < SIZE_PATH:
	new = last + 1
	add_link(last, new)
	last = new
	nodes.append(last)

add_link(last, 4)

# add nodes
while len(nodes) < total:
	last = last + 1
	nodes.append(last)

# generate links
total_links = int(total * random.uniform(1.5, 3.0))
while len(links) < total_links:
	node1 = random.randint(1, last)
	node2 = random.randint(1, last)
	if node1 == node2:
		continue
	else:
		if node1 < node2:
			add_link(node1, node2)
		else:
			add_link(node2, node1)

todest = random.randint(3, max(3, total/10))

while todest > 0:
	node = random.randint(1, last)
	if node != 4:
		todest = todest - 1
		if node == todest:
			continue
		if node < todest:
			add_link(node, todest)
		else:
			add_link(todest, node)

print "type route edge(node, node, int)."

while len(links) != 0:
	idx = random.randint(0, len(links)-1)
	item = links[idx]
	links.remove(item)
	distance = random.randint(1, 10)
	print "!edge(@" + str(item[0]) + ", @" + str(item[1]) + ", " + str(distance) + ")."
	print "!edge(@" + str(item[1]) + ", @" + str(item[0]) + ", " + str(distance) + ")."

