#!/usr/bin/python
#
# Generates a 3-colorable graph.

import sys
import random

if len(sys.argv) != 2:
	print "usage: python generate_colorable_graph.py <number of nodes>"
	sys.exit(1)

numnodes = int(sys.argv[1])

print "type route edge(node, node)."

connections = {}

for node in range(numnodes):
	connections[node] = []

def add_connection(node, other):
	connections[node].append(other)
	connections[other].append(currentnode)
	print "!edge(@" + str(currentnode) + ", @" + str(other) + "). !edge(@" + str(other) + ", @" + str(currentnode) + ")."


def add_edges(currentnode, edges):
	for edge in range(edges):
		tries = 0
		while True:
			if tries > 10:
				break
			other = random.randint(0, numnodes-1)
			tries = tries + 1
			if other == currentnode:
				continue
			if other in conns:
				continue
			otherconns = connections[other]
			if len(otherconns) >= 2:
			 	continue
			add_connection(node, other)
			break

currentnode = -1
while currentnode + 1 < numnodes:
	currentnode = currentnode + 1
	conns = connections[currentnode]
	if len(conns) == 2:
		continue
	if len(conns) == 1:
		edges = random.randint(0, 1)
	else:
		edges = random.randint(1, 2)
	add_edges(currentnode, edges)

#for k, v in connections.iteritems():
#	print k, len(v)
