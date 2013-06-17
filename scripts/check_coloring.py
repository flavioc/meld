#!/usr/bin/python

import sys
from lib_db import read_db

db = read_db(sys.stdin.readlines())

neighbors = {}

def add_neighbor(src, dst):
	try:
		mine = neighbors[src]
		if dst in mine:
			return
		else:
			mine.append(dst)
			try:
				other = neighbors[dst]
				other.append(src)
			except KeyError:
			 	neighbors[dst] = [src]
	except KeyError:
		neighbors[src] = [dst]
		try:
			other = neighbors[dst]
			other.append(src)
		except KeyError:
			neighbors[dst] = [src]

def print_neighbors():
	for src, dsts in neighbors.iteritems():
		print (src, dsts)

# add neighbors
for node, data in db.items():
	for d in data:
		name = d['name']
		args = d['args']
		if name == 'edge':
			add_neighbor(node, int(args[0].lstrip('@')))

colors = {}
# add colors
for node, data in db.items():
	checked = False
	for d in data:
		name = d['name']
		if name == 'color':
			checked = True
			args = d['args']
			color = int(args[0])
			if color == 0:
				print("Invalid color")
				sys.exit(1)
			try:
				older = colors[node]
				print ("Color already set for node", node)
				sys.exit(1)
			except:
				colors[node] = color
				try:
					edges = neighbors[node]
					for edge in edges:
						try:
							other = colors[edge]
							if other == color:
								print("Node " + str(node) + " has same color as " + str(edge))
								sys.exit(1)
						except:
							pass
				except:
					# All fine
					pass
	if checked is False:
		print("No color was set for node", node)
		sys.exit(1)

print("ALL OK")
