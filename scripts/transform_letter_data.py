#!/usr/bin/python

import csv
import sys

reader = csv.reader(open('letter-recognition.data', 'rb'), delimiter=',')

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
