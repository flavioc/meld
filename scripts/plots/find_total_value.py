#!/usr/bin/python

import sys

file = sys.argv[1]
first = True
total = 0

for line in open(file):
	if first:
		first = False
		continue
	vec = line.split(' ')
	vec.pop(0)
	for x in vec:
		total = total + int(x)

print total
