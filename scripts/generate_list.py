#!/usr/bin/python
# Generate list for quicksort.

import sys
import random

if len(sys.argv) != 2:
   print "usage: generate_list.py <size>"
   sys.exit(1)

size = int(sys.argv[1])

print "[",
for i in xrange(0, size):
   sys.stdout.write(str(random.randint(0, 100)))
   if i == size - 1:
      sys.stdout.write("]")
   else:
      sys.stdout.write(",")
