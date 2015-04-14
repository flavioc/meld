#!/usr/bin/python

import sys
import random
from lib import generate_graph
from lib import write_dedge

if len(sys.argv) < 4:
   print "generate_bfs.py <num nodes> <queries> <node query fraction> <threads?>"
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
using_threads = len(sys.argv) > 4
threads_list = []
for i in xrange(0, queries):
   target_node = nodes_select[random.randint(0, nodes_problem_total - 1)]
   start_node = random.randint(0, total - 1)
   if using_threads:
      threads_list.append(target_node)
   else:
      print "search(@" + str(start_node) + ", " + str(target_node) + ")."
if using_threads:
   s = "search(T, ["
   first = True
   for x in threads_list:
      if first:
         first = False
      else:
         s = s + ", "
      s = s + str(x)
   s = s + "])."
   print s
