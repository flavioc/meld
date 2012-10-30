#!/usr/bin/python
#
# This library reads a VM output and builds its own database.

def read_db(lines):
	current_node = None
	nodes = {}
	node_db = []
	for line in lines:
		line = line.rstrip('\n')
		if line.isdigit():
			if current_node is not None:
				nodes[current_node] = node_db
			current_node = int(line)
			node_db = []
		else:
			split1 = line.split('(')
			split2 = split1[1].split(')')
			name = split1[0]
			args = split2[0].split(', ')
			if len(args) == 1 and args[0] == '':
				args = []
			data = {'name': name, 'args': args}
			node_db.append(data)
	nodes[current_node] = node_db
	return nodes
