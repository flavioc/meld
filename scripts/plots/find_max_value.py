#!/usr/bin/python

import sys

file = sys.argv[1]
first = True
ret = 0

for line in open(file):
	if first:
		first = False
		continue
	vec = line.split(' ')
	vec.pop(0)
	for x in vec:
		x = int(x)
		if x > ret:
			ret = x

print ret
