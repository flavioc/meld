#!/usr/bin/python
#
# Gets count predicate and builds a colored grid based on that count.

import sys
import math
import cairo
from lib_db import read_db

if len(sys.argv) < 2:
	print "usage: python heat_color.py <file name>"
	sys.exit(1)

db = read_db(sys.stdin.readlines())

def read_count_node(data):
	for d in data:
		name = d['name']
		if name == 'count':
			return int(d['args'][0])
	return 0

def read_coord(data):
	for d in data:
		name = d['name']
		if name == '!coord':
			args = d['args']
			return (int(args[0]), int(args[1]))
	assert(False)
	return (0, 0)

def read_inner(data):
	for d in data:
		name = d['name']
		if name == '!inner':
			return True
	return False

counts = {}
coords = {}
inners = {}

maxcount = 0
totalcount = 0
maxx = 0
maxy = 0
lowestxinner = None
lowestyinner = None
highestxinner = None
highestyinner = None
for node, data in db.iteritems():
	total = read_count_node(data)
	if total > maxcount:
		maxcount = total
	totalcount = totalcount + total
	coord = read_coord(data)
	(x, y) = coord
	if x > maxx:
		maxx = x
	if y > maxy:
		maxy = y
	inner = read_inner(data)
	inners[node] = inner
	if inner:
		if lowestxinner is None or lowestxinner > x:
			lowestxinner = x
		if lowestyinner is None or lowestyinner > y:
			lowestyinner = y
		if highestxinner is None or highestxinner < x:
			highestxinner = x
		if highestyinner is None or highestyinner < y:
			highestyinner = y
	counts[node] = total
	coords[node] = coord

SCALE = 20
WIDTH = maxx * SCALE
HEIGHT = maxy * SCALE
surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, WIDTH, HEIGHT)
ctx = cairo.Context(surface)

for node, coord in coords.iteritems():
	count = counts[node]
	(x, y) = coord
	inner = inners[node]
	ratio = float(count) / float(maxcount)
	ctx.set_source_rgb(ratio, ratio, ratio)
	ctx.rectangle(x * SCALE, y * SCALE, SCALE, SCALE)
	ctx.fill()

ctx.rectangle(lowestxinner * SCALE, lowestyinner * SCALE, (highestxinner - lowestxinner + 1) * SCALE, (highestyinner - lowestyinner + 1) * SCALE)
ctx.set_source_rgb(1, 1, 1)
ctx.stroke()

ctx.select_font_face('Sans')
ctx.set_font_size(maxx / 2)
ctx.move_to(int(0.8 * WIDTH), HEIGHT - 20)
ctx.set_source_rgb(1.00, 0.83, 0.00) # yellow
ctx.show_text(str(maxcount))

ctx.select_font_face('Sans')
ctx.set_font_size(maxx / 2)
ctx.move_to(int(0.2 * WIDTH), HEIGHT - 20)
ctx.set_source_rgb(1.00, 0.00, 0.00) # red
ctx.show_text(str(totalcount))

surface.write_to_png(sys.argv[1])

