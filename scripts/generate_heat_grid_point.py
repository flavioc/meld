#!/usr/bin/python
#

import sys

from lib import write_dedgew, write_edgew

if len(sys.argv) < 8:
	print "Usage: generate_heat_grid_point.py <side> <heat side> <heat side weight> <weight other> <heat other> <hole size> <heat side heat>"
	sys.exit(1)
	
print 'type route edge(node, node, float).'

side = int(sys.argv[1])
heatside = float(sys.argv[2])
heatsideweight = float(sys.argv[3])
weightother = float(sys.argv[4])
heatother = float(sys.argv[5])
holesize = int(sys.argv[6])
heatsideheat = float(sys.argv[7])

limitrow = limitcol = side - 1

def inside_outer(row, col):
	if row >= 0 and row < outerside:
		return True
	if col >= 0 and col < outerside:
		return True
	if col >= (side - outerside) and col < side:
		return True
	if row >= (side - outerside) and row < side:
		return True

def inside_barrier(row, col):
	if row == col:
		half = side / 2
		half1 = half - holesize / 2
		if row <= half1:
			return True
		half1 = half + holesize / 2
		if row >= half1:
			return True
		return False
	else:
		return False

def inside_heat(row, col):
	half = side / 2
	if row < half and col > half:
		startrow = (half - heatside) / 2
		startcol = half + (half - heatside) / 2
		endrow = startrow + heatside
		endcol = startcol + heatside
		if row >= startrow and row <= endrow and col >= startcol and col <= endcol:
			return True
		else:
			return False
	else:
		return False
		

def write_weights(id, row, col, idother, rowother, colother):
	if inside_barrier(row, col):
		if inside_barrier(rowother, colother):
			write_dedgew(id, idother, 0.0)
		elif inside_heat(rowother, colother):
			write_dedgew(id, idother, 0.0)
		else:
			write_dedgew(id, idother, 0.0)
	elif inside_heat(row, col):
		if inside_barrier(rowother, colother):
			write_dedgew(id, idother, 0.0)
		elif inside_heat(rowother, colother):
			write_dedgew(id, idother, heatsideweight)
		else:
			write_dedgew(id, idother, weightother)
	else:
		if inside_barrier(rowother, colother):
			write_dedgew(id, idother, 0.0)
		elif inside_heat(rowother, colother):
			write_dedgew(id, idother, weightother)
		else:
			write_dedgew(id, idother, weightother)

def do_write_heat(id, heat):
	print "heat(@" + str(id) + ", " + str(heat) + ")."

def write_heat(id, row, col):
	if inside_barrier(row, col):
		do_write_heat(id, 0.0)
	elif inside_heat(row, col):
		do_write_heat(id, heatsideheat)
	else:
		do_write_heat(id, heatother)

for row in range(0, side):
   for col in range(0, side):
		other = inside_heat(row, col)
		barrier = inside_barrier(row, col)
		if other:
			assert(not barrier)
		if barrier:
			assert(not other)
		id = row * side + col
		write_heat(id, row, col)
		# south
		if row < limitrow:
			rowsouth = row + 1
			colsouth = col
			southid = rowsouth * side + colsouth
			write_weights(id, row, col, southid, rowsouth, colsouth)
		# west
		if col < limitcol:
			rowwest = row
			colwest = col + 1
			westid = rowwest * side + colwest
			write_weights(id, row, col, westid, rowwest, colwest)
