#!/usr/bin/python

import sys
from lib import write_edge
from lib import get_id
from lib import set_weight

def generate_tree(root, levels):
	if levels == 0:
		return
	left = get_id()
	right = get_id()
	write_edge(root, left)
	write_edge(root, right)
	generate_tree(left, levels - 1)
	generate_tree(right, levels - 1)

if len(sys.argv) < 2:
	print "Usage: generate_binary_tree.py <num levels> [weight]"
	sys.exit(1)


if len(sys.argv) == 3:
	set_weight(int(sys.argv[2]))
	print "type route edge(node, node, int)."
else:
	print "type route edge(node, node)."

levels = int(sys.argv[1])

generate_tree(get_id(), levels)
