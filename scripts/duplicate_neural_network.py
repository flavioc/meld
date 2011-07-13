#!/usr/bin/python

import csv
import sys

if len(sys.argv) != 4:
	print "usage: duplicate_neural_network.py <input file> <output file> <size input>"
	sys.exit(1)

input = sys.argv[1]
output = sys.argv[2]
size = int(sys.argv[3])

reader = csv.reader(open(input, 'rb'), delimiter=',')
writer = csv.writer(open(output, 'wb'), delimiter=',')

for row in reader:
	train = row[1:size+1]
	writer.writerow([row[0]] + train + row[1:])	
