#!/usr/bin/python

import sys
import random

if len(sys.argv) != 2:
   print "weighted_snap.py <txt file>"
   sys.exit(1)

random.seed(0)

for line in open(sys.argv[1], "r"):
   line = line.rstrip().lstrip()
   vec = line.split('\t')
   if len(vec) != 2:
      continue
   id1 = int(vec[0])
   id2 = int(vec[1])
   weight = random.randint(1, 500)
   print str(id1) + "\t" + str(id2) + "\t" + str(weight)
