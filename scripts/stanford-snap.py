#!/usr/bin/python
#
# Transforms files from http://snap.stanford.edu/data
# into Meld edge fact files.
#

import sys
from lib import write_edge
from lib import set_weight

if len(sys.argv) < 2:
   print "Usage: stanford-snap.py <file> [distance]"
   sys.exit(1)
   
if len(sys.argv) == 3:
   set_weight(int(sys.argv[2]))
else:
   set_weight(1)
   
f = open(sys.argv[1], "rb")

print "type route edge(node, node, int)."

for line in f:
   if line[0] == '#':
      continue
   vec = line.rstrip().split('\t')
   write_edge(vec[0], vec[1])