#!/usr/bin/python
#
# Converts tnet files into Meld edge files.
#
# http://toreopsahl.com/datasets/
#

import sys, random
from lib import write_edgew
from lib import set_weight

if len(sys.argv) < 2:
   print "Usage: transform_tnet_as_it_is.py <file>"
   sys.exit(1)

# read first data and find out how big the distances are
big_numbers = False
ones = True
f = open(sys.argv[1], "rb")
i = 0
for line in f:
	vec = line.rstrip().split(' ')
	d = int(vec[2])
	if d != 1:
		ones = False
	if d > 10000:
		big_numbers = True
		break
	i = i + 1
	if i > 10:
		break
   
f = open(sys.argv[1], "rb")

print "type route edge(node, node, int)."

for line in f:
	vec = line.strip().split(' ')
	d = int(vec[2])
	if big_numbers:
		d2 = d / 10000
		if d2 == 0:
			d2 = 1
	elif ones:
		d2 = random.randint(1, 10)
	else:
		d2 = d
	src = int(vec[0]) - 1
	dst = int(vec[1]) - 1
	write_edgew(src, dst, d2)
