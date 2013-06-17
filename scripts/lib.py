
import sys

weight = None
counter = 0

def get_id():
	global counter
	counter = counter + 1
	return counter - 1

def set_weight(new):
	global weight
	weight = new

def write_edge(a, b):
	global weight
	sys.stdout.write("!edge(@" + str(a) + ",@" + str(b))
	if weight is not None:
		sys.stdout.write("," + str(weight))
	print ")."

def write_dedge(a, b):
	write_edge(a, b)
	write_edge(b, a)
	
def write_edgew(a, b, w):
   sys.stdout.write("!edge(@" + str(a) + ",@" + str(b) + "," + ('{0:f}'.format(w)) + ").\n")

def write_dedgew(a, b, w):
	write_edgew(a, b, w)
	write_edgew(b, a, w)

def simple_write_edge(a, b):
	sys.stdout.write("!edge(@" + str(a) + ",@" + str(b) + ").\n")
