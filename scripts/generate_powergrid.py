#!/usr/bin/python
#
# Generates a power grid graph.

import sys
import random
import os

if len(sys.argv) != 3:
	print "usage: python generate_powergrid_graph.py <#Sinks> <#Sources>"
	sys.exit(1)

numsinks = int(sys.argv[1])
numsources = int(sys.argv[2])

for sink in range(numsinks):
	for sourcea in range(numsources):
		source = numsinks + sourcea
		print "!edge(@" + str(sink) + ", @" + str(source) + ")."

for sourcea in range(numsources):
	source = numsinks + sourcea
	for sink in range(numsinks):
		print "!edge(@" + str(source) + ", @" + str(sink) + ")."

def generate_sink():
	try:
		forced = os.environ['SINK']
		if forced is not None:
			return int(forced)
	except KeyError:
		pass
	return random.randint(1, 10)

for sink in range(numsinks):
	print "!sink(@" + str(sink) + ", " + str(generate_sink()) + ")."

def generate_source_capacity():
	try:
		forced = os.environ['SOURCE_CAPACITY']
		if forced is not None:
			return int(forced)
	except KeyError:
		pass
	return random.randint(5, 25)

for sourcea in range(numsources):
	source = numsinks + sourcea
	print "!source(@" + str(source) + ")."
	print "!capacity(@" + str(source) + ", " + str(generate_source_capacity()) + ")."

