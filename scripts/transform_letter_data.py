#!/usr/bin/python

import csv
import sys

if len(sys.argv) < 3:
	print "usage: transform_letter_data.py <input file> [letter]"
	sys.exit(1)

input = sys.argv[1]
reader = csv.reader(open(input, 'rb'), delimiter=',')

if len(sys.argv) == 3:
	END = sys.argv[2]
else:
	END = 'J'

for row in reader:
	letter = row[0]
	if ord(letter) <= ord(END):
		sys.stdout.write(",".join(row[1:]))
		for l in range(ord('A'),ord(END)+1):
			if l == ord(letter):
				sys.stdout.write(",1")
			else:
				sys.stdout.write(",0")
		print
