#!/usr/bin/python
#
# Generates a power grid graph.

import sys
import random
import os

if len(sys.argv) < 3:
	print "usage: python generate_powergrid.py <#Generators> <#Consumers>"
	sys.exit(1)

numgen = int(sys.argv[1])
numcons = int(sys.argv[2])

generators = [n for n in range(numgen)]
consumers = [n + numgen for n in range(numcons)]
power = [random.randint(1, 100) for _ in consumers]
total_power = sum(p for p in power)

print "const maxfails = " + str(numcons / 2) + "."
print "const all-generators = [@" + str(generators[0]),
for gen in generators[1:]:
   print ", @" + str(gen),
print "]."
print "const num-generators = " + str(len(generators)) + "."

for consumer, p in zip(consumers, power):
   print "!power(@" + str(consumer) + ", " + str(p) + ")."

rempower = power
conspergen = max(int(float(numcons)/float(numgen)), 1)
mingen = max(1, int(float(conspergen) * 0.5))
maxgen = int(float(conspergen) * 1.5)
count = 0
if len(sys.argv) > 3:
   factor = float(sys.argv[3])
else:
   factor = 1.1
for gen in generators:
   count = count + 1
   if count == len(generators):
      num = len(rempower)
   else:
      num = random.randint(mingen, maxgen)
   totalcap = 0
   for x in range(num):
      if rempower == []:
         break
      totalcap = totalcap + rempower[0]
      rempower = rempower[1:]
   totalcap = int(factor * float(totalcap))
   print "capacity(@" + str(gen) + ", " + str(totalcap) + ", 0)."
