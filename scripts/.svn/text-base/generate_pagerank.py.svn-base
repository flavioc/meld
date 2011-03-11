#!/usr/bin/python

import sys
import random

def list_has(list, x):
	try:
		list.index(x)
		return True
	except ValueError:
		return False

total = int(sys.argv[1])

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
		print str(i) + " " + str(link)
		
