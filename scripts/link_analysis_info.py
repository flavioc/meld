#!/usr/bin/python
#
# Info about adj_list files.
# http://www.cs.toronto.edu/~tsap/experiments/datasets/index.html
# into Meld source code files.
#

import sys

if len(sys.argv) != 2:
	print "usage: link_analysis_info.py <file>"
	sys.exit(1)

file = sys.argv[1]
f = open(file, 'rb')
nodes = {}
edge_count = 0
edges_per_node = {}

for line in f:
	line = line.rstrip('\n')
	vec = line.split(' ')
	id = int(vec[0].rstrip(':'))
	nodes[id] = True
	size = len(vec)-2
	edges_per_node[id] = size
	if size > 0:
		for dest in vec[1:len(vec)-1]:
			dest = int(dest)
			nodes[dest] = True
			edge_count = edge_count + 1

node_count = len(nodes)
ls = []
for key, value in edges_per_node.items():
	ls.append(value)
ls.sort()
ls.reverse()

half = edge_count / 2
quarter = half / 2
thirds = edge_count * 0.66667
acc = 0
total = 0
half_total = None
quarter_total = None
thirds_total = None
for x in ls:
	acc = acc + x
	total = total + 1
	if half_total is None and acc >= half:
		half_total = total
	if quarter_total is None and acc >= quarter:
		quarter_total = total
	if thirds_total is None and acc >= thirds:
		thirds_total = total
print "node total", node_count
print "edge total", edge_count
print "edges per node", float(edge_count) / float(node_count)
print "half count", half_total
print "quarter count", quarter_total
print "thirds count", thirds_total
