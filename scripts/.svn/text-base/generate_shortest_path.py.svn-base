#!/usr/bin/python

import sys
import random

def list_has(list, x):
	try:
		list.index(x)
		return True
	except ValueError:
		return False

def generate_weight():
	return random.randint(1, 10)

total = int(sys.argv[1])
source = random.randint(1, total)
dest = source
while (dest == source):
	dest = random.randint(1, total)

print "GRAPH : ", str(source), " -> ", str(dest)

for i in range(total):
	links = random.randint(1, int(total*0.75))
	list = []
	for j in range(links):
		link = random.randint(1, total - 1)
		if link == i:
			link = (link + 1) % (total - 1)
		if not list_has(list, link):
			list.append(link)
	for link in list:
		weight = generate_weight()
		print str(i) + " " + str(link) + " [" + str(weight) + "]"
		
