#!/usr/bin/python
#
# Sums all the values from a certain predicate from the database.

import sys
from lib_db import read_db

if len(sys.argv) < 3:
	print "usage: count_db_total.py <predicate> <argument position (1..N)>"
	sys.exit(1)

predicate = sys.argv[1]
position = int(sys.argv[2])

db = read_db(sys.stdin.readlines())
total = 0

for node, data in db.iteritems():
	for d in data:
		name = d['name']
		if name == 'count':
			total = total + int(d['args'][position-1])
			
print total
