#!/usr/bin/python

import sys

print "type route edge(node, node)."

counter = 0

def get_id():
	global counter
	counter = counter + 1
	return counter - 1

def write_edge(a, b):
	print "edge(@" + str(a) + ",@" + str(b) + ")."

def generate_tree(root, levels):
	if levels == 0:
		return
	left = get_id()
	right = get_id()
	write_edge(root, left)
	write_edge(root, right)
	generate_tree(left, levels - 1)
	generate_tree(right, levels - 1)

levels = int(sys.argv[1])

generate_tree(get_id(), levels)
