#!/usr/bin/python
#
# Generates a power grid graph.

import sys
import random
import os

if len(sys.argv) != 6:
	print "usage: python generate_powergrid.py <#Generators> <#Consumers> <factor> <max fails> <expand factor>"
	sys.exit(1)

random.seed(0)
numgen = int(sys.argv[1])
numcons = int(sys.argv[2])

generators = [n for n in range(numgen)]
consumers = [n + numgen for n in range(numcons)]
power = [random.randint(1, 100) for _ in consumers]
total_power = sum(p for p in power)

print "const maxfails = " + sys.argv[4] + "."
print "const all-generators = [@" + str(generators[0]),
for gen in generators[1:]:
   print ", @" + str(gen),
print "]."
print "const num-generators = " + str(len(generators)) + "."

count = 0
for consumer, p in zip(consumers, power):
   print "!power(@" + str(consumer) + ", " + str(p) + ")."
   print "!consumer-id(@" + str(consumer) + ", " + str(count) + ")."
   count = count + 1

rempower = power
conspergen = max(int(float(numcons)/float(numgen)), 1)
expandfactor = float(sys.argv[5])
mingen = max(1, int(float(conspergen) * (max(0.0, 1.0 - expandfactor))))
maxgen = int(float(conspergen) * (1.0 + expandfactor))
count = 0
if len(sys.argv) > 3:
   factor = float(sys.argv[3])
else:
   factor = 1.1
for gen in generators:
   print "!generator-id(@" + str(gen) + ", " + str(count) + ")."
   count = count + 1
   if count == len(generators):
      num = len(rempower)
   else:
      num = random.randint(mingen, maxgen)
   items = rempower[0:num]
   totalcap = int(factor * float(sum(items)))
   rempower = rempower[num:]
   print "capacity(@" + str(gen) + ", " + str(totalcap) + ", 0)."
