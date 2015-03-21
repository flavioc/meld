#!/usr/bin/python
#
# Transforms files from http://snap.stanford.edu/data
# into Meld edge fact files.
#

import sys
import random
from lib import write_edgew

if len(sys.argv) < 2:
   print "Usage: stanford-snap.py <file> [distance]"
   sys.exit(1)
   
f = open(sys.argv[1], "rb")

idmaps = {}
newid = 0
def map_id(id):
   global newid, idmaps
   try:
      ret = idmaps[id]
      return ret
   except KeyError:
      idmaps[id] = newid
      newid = newid + 1
      return newid - 1

print "type route edge(node, node, float)."

for line in f:
   if not line:
      continue
   if line[0] == '#':
      continue
   vec = line.rstrip().split('\t')
   id1 = map_id(int(vec[0]))
   id2 = map_id(int(vec[1]))
   if len(sys.argv) == 2:
      write_edgew(id1, id2, int(random.randint(1, 500)))
   else:
      write_edgew(id1, id2, int(sys.argv[2]))
