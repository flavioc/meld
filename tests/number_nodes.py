#!/usr/bin/python

import sys
import struct

if len(sys.argv) < 2:
   print "-1"
   sys.exit(1)

filename = sys.argv[1]

with open(filename, "rb") as in_file:
   in_file.seek(17)
   print struct.unpack('i', in_file.read(4))[0]
