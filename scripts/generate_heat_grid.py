#!/usr/bin/python
#
# Generates a grid where there's an outer layer with different weight than the inner layer

import sys

from lib import write_dedgew, write_edgew

if len(sys.argv) < 6:
	print "Usage: generate_heat_grid.py <side> <outer side> <outer weight> <inner weight> <transition weight>"
	sys.exit(1)
	
print 'type route edge(node, node, float).'

side = int(sys.argv[1])
outerside = int(sys.argv[2])
outerweight = float(sys.argv[3])
innerweight = float(sys.argv[4])
transitionweight = float(sys.argv[5])

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

for row in range(0, side):
	for col in range(0, side):
		isouter = inside_outer(row, col)
		id = row * side + col
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
