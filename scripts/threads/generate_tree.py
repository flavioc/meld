#!/usr/bin/python
# vim: tabstop=3 expandtab shiftwidth=3 softtabstop=3

import sys
import math
import random

if len(sys.argv) != 4:
   print "generate_tree.py <levels> <queries> <node query fraction>"
   sys.exit(0)

levels = int(sys.argv[1])
nodes = math.pow(2, levels) - 1
idcount = 0

node_map = {}

def generate_tree(minimum, maximum, parent, left, hasparent=False):
   global idcount, node_map
   if minimum > maximum:
      return
   id = int((maximum-minimum)/2 + minimum)
   myid = idcount
   node_map[id] = myid
   if hasparent:
      if left:
         print "!left(@" + str(parent) + ", @" + str(myid) + ")."
      else:
         print "!right(@" + str(parent) + ", @" + str(myid) + ")."
   value = random.randint(1, 100)
   print "value(@" + str(myid) + ", " + str(id) + ", " + str(value) + ")."
   idcount = idcount + 1
   generate_tree(minimum, id-1, myid, True, True)
   generate_tree(id+1, maximum, myid, False, True)

generate_tree(0, nodes-1, None, False)

node_fraction = float(sys.argv[3])
nodes_problem_total = int(float(node_fraction) * float(nodes))
remaining_nodes = [key for key, value in node_map.iteritems()]
random.shuffle(remaining_nodes)
nodes_select = []
while len(nodes_select) != nodes_problem_total:
   nodes_select.append(remaining_nodes.pop())

# generate problem instances.
queries = int(sys.argv[2])
queries_map = {}
for i in xrange(0, queries):
   idx = random.randint(0, nodes_problem_total - 1)
   target_node = nodes_select[idx]
   target_id = node_map[target_node]
   look = random.randint(0, 2)
   try:
      query = queries_map[target_id]
      if look <= 1:
         print "do-lookup(@" + str(target_id) + ", " + str(target_node) + ", " + str(query + 1) + ")."
      else:
         num = random.randint(0, 10000)
         print "do-replace(@" + str(target_id) + ", " + str(target_node) + ", " + str(num) + ", " + str(query + 1) + ")."
      queries_map[target_id] = query + 1
   except KeyError:
      if look <= 1:
         print "lookup(@0, " + str(target_node) + ", 0)."
      else:
         num = random.randint(0, 10000)
         print "replace(@0, " + str(target_node) + ", " + str(num) + ", 0)."
      queries_map[target_id] = 0

