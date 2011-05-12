#!/usr/bin/python

import sys
import random

def list_has(list, x):
	try:
		list.index(x)
		return True
	except ValueError:
		return False

def hash_has(hash, x):
	try:
		hash[x]
		return True
	except KeyError:
		return False

def has_link(hash, k, v):
	try:
		r = hash[k]
		return list_has(r, v)
	except KeyError:
		return False

def add_link(hash, k, v):
	try:
		r = hash[k]
		if not list_has(r, v):
			r.append(v)
	except KeyError:
		hash[k] = [v]

def generate_weight():
	return random.randint(1, 10)

if len(sys.argv) < 2:
	print "Usage: generate_shortest_path.py <number of nodes> [-all-pairs]"
	sys.exit(1)

all_pairs = False
if len(sys.argv) == 3:
	arg3 = sys.argv[2]
	if arg3 == '-all-pairs':
		all_pairs = True

total = int(sys.argv[1])

print "type route edge(node, node, int)."

if not all_pairs:
	source = random.randint(0, total-1)
	dest = source
	while (dest == source):
		dest = random.randint(0, total-1)

	print "type start(node)."
	print "type end(node)."

	print "start(@" + str(source) + ")."
	print "end(@" + str(dest) + ")."

alllinks = {}

for node in range(total):
	links = random.randint(1, int(total*0.75))
	for j in range(links):
		link = random.randint(1, total - 1)
		if link == node:
			link = (link + 1) % (total - 1)
		add_link(alllinks, node, link)
		if random.randint(0, 1) == 1:
			add_link(alllinks, link, node)

for k, v in alllinks.iteritems():
	for link in v:
		weight = generate_weight()
		print "edge(@" + str(k) + ", @" + str(link) + ", " + str(weight) + ")."
	
