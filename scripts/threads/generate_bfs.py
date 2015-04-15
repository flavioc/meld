#!/usr/bin/python

import sys
import random
from lib import generate_graph
from lib import write_dedge
from lib import write_list

if len(sys.argv) < 5:
   print "generate_bfs.py <num nodes> <queries> <node source fraction> <node target fraction>"
   sys.exit(1)

random.seed(0)

total = int(sys.argv[1])
nodes = generate_graph(total, False)

for node in range(total):
   print "value(@" + str(node) + "," + str(node) + ")."
    
for node, links in nodes.iteritems():
    for link in links:
       write_dedge(node, link)

node_fraction = int(sys.argv[3])
nodes_problem_total = int(float(node_fraction)/100.0 * float(total))
remaining_nodes = [node for node in range(total)]
random.shuffle(remaining_nodes)
nodes_select = []
while len(nodes_select) != nodes_problem_total:
   nodes_select.append(remaining_nodes.pop())

queries = int(sys.argv[2])
target_fraction = int(sys.argv[4])
fraction_total = int(float(target_fraction)/100.0 * float(total))
threads_list = []
for i in xrange(0, queries):
   target_nodes = []
   for j in xrange(fraction_total):
      target_node = nodes_select[random.randint(0, nodes_problem_total - 1)]
      if target_node not in target_nodes:
         target_nodes.append(target_node)
   start_node = None
   while start_node is None or start_node in target_nodes:
      start_node = random.randint(0, total - 1)
   print "search(@" + str(start_node) + ", " + str(i) + ", " + write_list(target_nodes) + ")."
