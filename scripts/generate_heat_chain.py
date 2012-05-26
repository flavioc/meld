#!/usr/bin/python

import sys

from lib import write_dedgew

if len(sys.argv) < 2:
	print "Usage: generate_heat_chain.py <num nodes> [drop type (either linear (by default), quadratic, constant)]"
	sys.exit(1)
	
drop_type = ''
drop_constant = 0

print 'type route edge(node, node, float).'

nodes = int(sys.argv[1])

if len(sys.argv) == 3:
	drop_type = sys.argv[2]
	if drop_type[:8] == 'constant':
		drop_part = drop_type[8:]
		drop_constant = float(drop_part)
		drop_type = 'constant'
else:
	drop_type = 'linear'

# if drop is linear, we can calculate the constant and make it 'constant'
if drop_type == 'linear':
	drop_constant = 1.0 / float(nodes)
	drop_type = 'constant'

previous = 0

if drop_type == 'constant':
	weight = 1.0
	for i in range(1, nodes):
		new = i 
		# from previous to new
		write_dedgew(previous, new, weight)
		weight = weight - drop_constant
		if weight < 0.0:
			weight = 0.0 # keep it above 0.0
		previous = new
elif drop_type == 'quadratic':
	base = float(nodes) * float(nodes)
	# expression: x^2/base
	for i in range(1, nodes):
		new = i
		# we must compute the inverse node number
		# if 0 then it must be N
		# if 1 then it must be N - 1
		inverse = nodes - previous
		weight = float(inverse * inverse) / base
		write_dedgew(previous, new, weight)
		previous = new