#!/usr/bin/python
#
# Sums all the values from a certain predicate from the database.

import sys

if len(sys.argv) < 3:
	print "usage: count_db_total.py <predicate> <argument position (1..N)>"
	sys.exit(1)

predicate = sys.argv[1]
lenpred = len(predicate)
position = int(sys.argv[2])

data = sys.stdin.readlines()
total = 0

for line in data:
	part = line[:lenpred]
	if part == predicate:
		if line[lenpred] == '(':
			arg = line.split("(")[1].split(')')[0].split(',')[position-1]
			total = total + int(arg)
			
print total