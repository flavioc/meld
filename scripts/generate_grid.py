#!/usr/bin/python

import sys

from lib import write_edge
from lib import set_weight

WRITE_COORD = True
WRITE_INOUT = False

if len(sys.argv) < 2:
	print "Usage: generate_grid.py <num nodes> [weight]"
	sys.exit(1)

if len(sys.argv) == 3:
	set_weight(int(sys.argv[2]))
	#print "type route edge(node, node, int)."
#else:
#print "type route edge(node, node)."

arg = sys.argv[1]
lenvec = arg.split('@', 2)

if len(lenvec) == 2:
   width = int(lenvec[0])
   height = int(lenvec[1])
else:
   width = int(lenvec[0])
   height = width

def write_conn(id1, id2, w):
	global WRITE_INOUT
	if WRITE_INOUT:
		print "!output(@" + str(id1) + ", @" + str(id2) + ", " + str(w) + ")."
		print "!input(@" + str(id2) + ", @" + str(id1) + ", " + str(w) + ")."
	else:
		write_edge(id1, id2)

def num_edges(row, col):
	global width, height
	if row == 0:
		if col == 0:
			return 2
		elif col == height - 1:
			return 2
		else:
			return 3
	elif row == width - 1:
		if col == 0:
			return 2
		elif col == height - 1:
			return 2
		else:
			return 3
	else:
		if col == 0:
			return 3
		elif col == height - 1:
			return 3
		else:
			return 4

def write_line(row):
	for col in range(0, width):
		id = row * width + col
		global WRITE_COORD
		if WRITE_COORD:
			print "!coord(@" + str(id) + ", " + str(row) + ", " + str(col) + ")."
		edges_id = float(num_edges(row, col))
		if row < height - 1:
			southid = id + width
			edges_southid = float(num_edges(row + 1, col))
			write_conn(southid, id, 1.0 / edges_id)
			write_conn(id, southid, 1.0 / edges_southid)
		if col < width - 1:
			eastid = id + 1
			edges_eastid = float(num_edges(row, col + 1))
			write_conn(eastid, id, 1.0 / edges_id)
			write_conn(id, eastid, 1.0 / edges_eastid)
			
for row in range(0, height):
	write_line(row)
