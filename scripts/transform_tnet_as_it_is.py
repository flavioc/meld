#!/usr/bin/python
#
# Converts tnet files into Meld edge files.
#
# http://toreopsahl.com/datasets/
#

import sys
from lib import write_edgew
from lib import set_weight

if len(sys.argv) < 2:
   print "Usage: tnet.py <file>"
   sys.exit(1)
   
f = open(sys.argv[1], "rb")

print "type route edge(node, node, int)."

for line in f:
   vec = line.rstrip().split(' ')
   write_edgew(vec[0], vec[1], vec[2])