#!/usr/bin/python
#
# Generates a grid where there's an outer layer with different weight than the inner layer

import sys

from lib import write_dedgew, write_edgew

WRITE_COORDS = True
ONE_PER_THREAD = False

if len(sys.argv) < 8:
	print "Usage: generate_heat_grid.py <side> <outer side> <outer weight> <inner weight> <transition weight> <inside heat> <outside heat> [threads]"
	sys.exit(1)
	
print 'type inbound(node, int).'
print 'type route edge(node, node, float).'
print 'type coord(node, int, int).'
print 'type inner(node).'
print 'type linear heat(node, float).'

side = int(sys.argv[1])
outerside = int(sys.argv[2])
outerweight = float(sys.argv[3])
innerweight = float(sys.argv[4])
transitionweight = float(sys.argv[5])
insideheat = float(sys.argv[6])
outsideheat = float(sys.argv[7])
if len(sys.argv) > 8:
	numthreads = int(sys.argv[8])
else:
	numthreads = 1

limitrow = limitcol = side-1

def inside_outer(row, col):
	if row >= 0 and row < outerside:
		return True
	if col >= 0 and col < outerside:
		return True
	if col >= (side - outerside) and col < side:
		return True
	if row >= (side - outerside) and row < side:
		return True

def write_weights(id, otherid, isouter, isotherouter):
	if isotherouter and isouter:
		write_dedgew(id, otherid, outerweight)
	elif isotherouter and not isouter:
		write_dedgew(id, otherid, transitionweight)
	elif not isotherouter and isouter:
		write_dedgew(id, otherid, transitionweight)
	elif not isotherouter and not isouter:
		write_dedgew(id, otherid, innerweight)
	else:
		print "PROBLEM"

def write_heat(id, heat):
	print "heat(@" + str(id) + ", " + str(heat) + ")."

def write_coord(id, row, col):
	print "!coord(@" + str(id) + ", " + str(row) + ", " + str(col) + ")."

def write_inner(id):
	print "!inner(@" + str(id) + ")."

def write_inbound(id, number):
	print "!inbound(@" + str(id) + ", " + str(number) + ")."

def do_node(row, col):
	isouter = inside_outer(row, col)
	id = row * side + col
	write_inbound(id, 4)
	if WRITE_COORDS:
		write_coord(id, row, col)
	if isouter:
		write_heat(id, outsideheat)
	else:
		if WRITE_COORDS:
			write_inner(id)
		write_heat(id, insideheat)

def do_weights(row, col):
	id = row * side + col
	isouter = inside_outer(row, col)
	# south
	if row < limitrow:
		rowsouth = row + 1
		colsouth = col
		issouthouter = inside_outer(rowsouth, colsouth)
		southid = rowsouth * side + colsouth
		write_weights(id, southid, isouter, issouthouter)
	# west
	if col < limitcol:
		rowwest = row
		colwest = col + 1
		iswestouter = inside_outer(rowwest, colwest)
		westid = rowwest * side + colwest
		write_weights(id, westid, isouter, iswestouter)

#for row in range(outerside, side - outerside):
#   for col in range(outerside, side - outerside):
#      assert(not inside_outer(row, col))
#      do_node(row, col)

def divisible_by(x, y):
	return x % y == 0

def do_range(startrow, endrow, startcol, endcol):
	for row in range(startrow, endrow):
		for col in range(startcol, endcol):
			do_node(row, col)

def do_blocks(thread):
	current = 0
	fifth = side / 5
	for row in range(0, fifth):
		for col in range(0, fifth):
			if current == thread:
				do_range(row * 5, (row + 1) * 5, col * 5, (col + 1) * 5)
			current = current + 1
			if current == numthreads:
				current = 0

if ONE_PER_THREAD:
	if numthreads == 1:
		do_range(0, side, 0, side)
	elif numthreads == 2:
		do_range(0, side, 0, side / 2)
		do_range(0, side, side / 2, side)
	elif numthreads == 4:
		do_range(0, side / 2, 0, side / 2)
		do_range(0, side / 2, side / 2, side)
		do_range(side / 2, side, 0, side / 2)
		do_range(side / 2, side, side / 2, side)
	elif numthreads == 6:
		pass
	elif numthreads == 8:
		quarter = side / 4
		half = side / 2
		do_range(0, half, 0, quarter)
		do_range(0, half, quarter, 2 * quarter)
		do_range(0, half, 2 * quarter, 3 * quarter)
		do_range(0, half, 3 * quarter, side)
		do_range(half, side, 0, quarter)
		do_range(half, side, quarter, 2 * quarter)
		do_range(half, side, 2 * quarter, 3 * quarter)
		do_range(half, side, 3 * quarter, side)
	elif numthreads == 10:
		assert(divisible_by(side, 5))
		assert(divisible_by(side, 2))
		fifth = side / 5
		half = side / 2
		do_range(0, half, 0, fifth)
		do_range(0, half, fifth, 2 * fifth)
		do_range(0, half, 2 * fifth, 3 * fifth)
		do_range(0, half, 3 * fifth, 4 * fifth)
		do_range(0, half, 4 * fifth, 5 * fifth)
		do_range(half, side, 0, fifth)
		do_range(half, side, fifth, 2 * fifth)
		do_range(half, side, 2 * fifth, 3 * fifth)
		do_range(half, side, 3 * fifth, 4 * fifth)
		do_range(half, side, 4 * fifth, 5 * fifth)
	elif numthreads == 12:
		pass
	elif numthreads == 14:
		pass
	elif numthreads == 16:
		assert(divisible_by(side, 4))
		quarter = side / 4
		for i in range(0, 4):
			do_range(i * quarter, (i + 1) * quarter, 0, quarter)
			do_range(i * quarter, (i + 1) * quarter, quarter, 2 * quarter)
			do_range(i * quarter, (i + 1) * quarter, 2 * quarter, 3 * quarter)
			do_range(i * quarter, (i + 1) * quarter, 3 * quarter, 4 * quarter)
else:
	if numthreads == 1:
		do_range(0, side, 0, side)
	elif numthreads == 2:
		do_blocks(0)
		do_blocks(1)
	elif numthreads == 4:
		do_blocks(0)
		do_blocks(1)
		do_blocks(2)
		do_blocks(3)
	elif numthreads == 8:
		do_blocks(0)
		do_blocks(1)
		do_blocks(2)
		do_blocks(3)
		do_blocks(4)
		do_blocks(5)
		do_blocks(6)
		do_blocks(7)
	elif numthreads == 16:
		do_blocks(0)
		do_blocks(1)
		do_blocks(2)
		do_blocks(3)
		do_blocks(4)
		do_blocks(5)
		do_blocks(6)
		do_blocks(7)
		do_blocks(8)
		do_blocks(9)
		do_blocks(10)
		do_blocks(11)
		do_blocks(12)
		do_blocks(13)
		do_blocks(14)
		do_blocks(15)

for row in range(0, side):
	for col in range(0, side):
		do_weights(row, col)

sys.exit(0)

