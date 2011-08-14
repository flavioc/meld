
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
	sys.stdout.write("edge(@" + str(a) + ",@" + str(b))
	if weight is not None:
		sys.stdout.write("," + str(weight))
	print ")."
	
def write_edgew(a, b, w):
   sys.stdout.write("edge(@" + str(a) + ",@" + str(b) + "," + str(w) + ").\n")

def simple_write_edge(a, b):
	sys.stdout.write("edge(@" + str(a) + ",@" + str(b) + ").\n")
