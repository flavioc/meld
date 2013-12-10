#!/usr/bin/python

import sys

if len(sys.argv) != 2:
	sys.exit(1)

def rchop(thestring, ending):
	if thestring.endswith(ending):
		return thestring[:-len(ending)]
	return thestring

instrs = {}
for line in open(sys.argv[1]):
	line = line.strip()
	vec = line.split(' ')
	if len(vec) > 0:
		instr0 = vec[0].strip()
		vec2 = instr0.split('=')
		instr = vec2[0].strip()
		bc = vec[len(vec)-1].strip()
		vec3 = bc.split('=')
		bc = vec3[len(vec3) - 1]
		bc = rchop(bc, ",").lower()
		bc = int(bc, 16)
		instrs[bc] = instr

sys.stdout.write("static void *jump_table[256] = {")
for i in range(0,256):
	try:
		instr = instrs[i]
		label = rchop(instr, "_INSTR").lower().strip()
		sys.stdout.write("&&" + label)
	except KeyError:
		sys.stdout.write("NULL")
	if i < 255:
	 	sys.stdout.write(", ")
sys.stdout.write("};\n")
sys.stdout.flush()
