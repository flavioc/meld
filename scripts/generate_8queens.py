#!/usr/bin/python

import sys

if len(sys.argv) < 2:
	print "Usage: generate-8queens.py <side> [h | v] [sb]"
	sys.exit(1)

side = int(sys.argv[1])
if len(sys.argv) > 2:
	style = sys.argv[2]
else:
	style = None

use_bottom_priority = False
use_top_priority = False
use_static = False
if len(sys.argv) == 4:
   if 'b' in sys.argv[3]:
      use_bottom_priority = True
   if 't' in sys.argv[3]:
      use_top_priority = True
   if 's' in sys.argv[3]:
      use_static = True

def map_node(id):
	if style is None or style == 'h':
		return id # horizontal
	row = id / side
	col = id % side
	if style == 'v':
		return row + col * side # vertical
	assert(False)
	return 0

print "const size = " + str(side) + "."

for x in range(0, side):
	for y in range(0, side):
		id = x * side + y
		print "!coord(@" + str(map_node(id)) + ", " + str(x) + ", " + str(y) + ")."
		# generate down right
		if y >= side - 2:
			print "!down-right(@" + str(map_node(id)) + ", @" + str(map_node(id)) + ")."
		else:
			if x == side - 1:
				print "!down-right(@" + str(map_node(id)) + ", @" + str(map_node(id)) + ")."
			else:
				print "!down-right(@" + str(map_node(id)) + ", @" + str(map_node(id + side + 2)) + ")."

		# generate down left
		if y <= 1:
			print "!down-left(@" + str(map_node(id)) + ", @" + str(map_node(id)) + ")."
		else:
			if x == side - 1:
				print "!down-left(@" + str(map_node(id)) + ", @" + str(map_node(id)) + ")."
			else:
				print "!down-left(@" + str(map_node(id)) + ", @" + str(map_node(id + side - 2)) + ")."

		if y != 0:
			print "!left(@" + str(map_node(id)) + ", @" + str(map_node(id - 1)) + ")."
		else:
			print "!left(@" + str(map_node(id)) + ", @" + str(map_node(id)) + ")."
		if y != side - 1:
			print "!right(@" + str(map_node(id)) + ", @" + str(map_node(id + 1)) + ")."
		else:
			print "!right(@" + str(map_node(id)) + ", @" + str(map_node(id)) + ")."

if use_bottom_priority:
   for x in range(0, side):
      for y in range(0, side):
         id = x * side + y
         print "set-default-priority(@" + str(map_node(id)) + ", " + str(x + 1) + ".0)."
if use_top_priority:
   for x in range(0, side):
      for y in range(0, side):
         id = x * side + y
         print "set-default-priority(@" + str(map_node(id)) + ", " + str(side - x) + ".0)."
if use_static:
   for x in range(0, side):
      for y in range(0, side):
         id = x * side + y
         print "set-cpu(@" + str(map_node(id)) + ", partition_vertical(" + str(x) + ", " + str(y) + ", " + str(side) + ", " + str(side) + "))."

