#!/usr/bin/python
# vim: tabstop=3 expandtab shiftwidth=3 softtabstop=3

import sys
import math
import random

if len(sys.argv) != 4:
   print "generate_tree.py <levels> <queries> <node query fraction>"
   sys.exit(0)

levels = int(sys.argv[1])
nodes = int(math.pow(2, levels)) - 1
OUTPUT = True
AVERAGE = False

# maps keys to node ids
node_map = {}

def generate_tree(index, level, minimum, maximum, parent, left, hasparent=False):
   global node_map
   if minimum > maximum:
      return
   id = int((maximum-minimum)/2 + minimum)
   myid = int(math.pow(2, level)) + index - 1
   node_map[id] = myid
   if hasparent and OUTPUT:
      if left:
         print "!left(@" + str(parent) + ", @" + str(myid) + ")."
      else:
         print "!right(@" + str(parent) + ", @" + str(myid) + ")."
   value = random.randint(1, 100)
   if OUTPUT:
      print "value(@" + str(myid) + ", " + str(id) + ", " + str(value) + ")."
   generate_tree(index * 2, level + 1, minimum, id-1, myid, True, True)
   generate_tree(index * 2 + 1, level + 1, id+1, maximum, myid, False, True)

generate_tree(0, 0, 0, nodes-1, None, False)
random.seed(0)

node_fraction = float(sys.argv[3])
nodes_problem_total = int(float(node_fraction) * float(nodes))
range_nodes = max(nodes/2, nodes_problem_total)
remaining_nodes = [nodes - 1 - key for key in range(range_nodes)]
random.shuffle(remaining_nodes)
nodes_select = []
mirror_counter = int(nodes)
remaining = nodes_problem_total
while remaining > 0:
   remaining = remaining - 1
   key = remaining_nodes.pop()
   nodes_select.append((key, mirror_counter))
   if OUTPUT:
      print "!mirror(@" + str(node_map[key]) + ", @" + str(mirror_counter) + ")."
   mirror_counter = mirror_counter + 1

# generate problem instances.
queries = int(sys.argv[2])
queries_map = {}
for i in xrange(0, queries):
   idx = random.randint(0, nodes_problem_total - 1)
   (target_node, target_id) = nodes_select[idx]
   look = random.randint(0, 2)
   try:
      query = queries_map[target_id]
      if OUTPUT:
         if look <= 1:
            print "do-lookup(@" + str(target_id) + ", " + str(target_node) + ", " + str(query + 1) + ")."
         else:
            num = random.randint(0, 10000)
            print "do-replace(@" + str(target_id) + ", " + str(target_node) + ", " + str(num) + ", " + str(query + 1) + ")."
      queries_map[target_id] = query + 1
   except KeyError:
      if OUTPUT:
         if look <= 1:
            print "lookup(@0, " + str(target_node) + ", 0)."
         else:
            num = random.randint(0, 10000)
            print "replace(@0, " + str(target_node) + ", " + str(num) + ", 0)."
      queries_map[target_id] = 0

if AVERAGE:
   total = 0
   nodes_total = 0
   for node, val in queries_map.iteritems():
      total = total + val + 1
      nodes_total = nodes_total + 1
   print float(total)/float(nodes_total)

