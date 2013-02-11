
#!/usr/bin/python

import sys
import cairo
import random
import math

def is_number(s):
	try:
		int(s)
		return True
	except ValueError:
		return False

if len(sys.argv) != 2:
	print "usage: python active_inactive.py <file.state>"
	sys.exit(1)

fp = open(sys.argv[1], 'r')

lines = fp.readlines()[1:]
maxqueue = 0
numcolumns = 0
for line in lines:
	line = line.rstrip('\n')
	vec = line.split(' ')[1:]
	if numcolumns == 0:
		numcolumns = len(vec)
	for v in vec:
		if is_number(v):
			v = int(v)
			if v > maxqueue:
				maxqueue = v
timetotal = len(lines)

WIDTH = min(400, 1 * timetotal)
HEIGHT = 64 * numcolumns
surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, WIDTH, HEIGHT)
ctx = cairo.Context(surface)
ctx.set_source_rgb(1, 1, 1)
ctx.rectangle(0, 0, WIDTH, HEIGHT)
ctx.fill()

heightdivision = HEIGHT/numcolumns
for i in range(numcolumns-1):
	height = (i + 1) * heightdivision
#ctx.set_source_rgb(0, 0, 0)
#ctx.rectangle(0, height, WIDTH, 1)
#ctx.fill()

widthgap = 10
widthavailable = WIDTH - widthgap
widthdivision = widthavailable/float(timetotal)
currentwidth = widthgap / 2.0
time = 0
for line in lines:
	line = line.rstrip('\n')
	vec = line.split(' ')[1:]
	thrnum = 0
	gap = heightdivision / 3.0
	startwidth = int(widthgap/2.0 + widthdivision * time)
	endwidth = startwidth + int(widthdivision)
	startwidth = currentwidth
	widthlength = endwidth - startwidth
	currentwidth = endwidth
	for thr in vec:
		if thr == 'i':
			ctx.set_source_rgb(1, 1, 0)
		elif thr == 's':
			ctx.set_source_rgb(0, 0, 1)
		elif thr == 'r':
			ctx.set_source_rgb(1, 1, 0)
		elif is_number(thr):
			v = int(thr)
			if v == 0:
				ctx.set_source_rgb(1, 1, 0)
			else:
				ratio = float(thr) /float(maxqueue)
				ctx.set_source_rgb(ratio, 0, 0)
		else:
			print thr
			assert(false)
		ctx.rectangle(startwidth, thrnum * heightdivision + gap/2.0, widthlength, heightdivision - gap)
		ctx.fill()
		thrnum = thrnum + 1
	time = time + 1
		
ctx.set_source_rgb(0, 0, 0)
ctx.rectangle(0, HEIGHT - 5, WIDTH, 5)
ctx.fill()
surface.write_to_png(sys.argv[1] + '.png')
