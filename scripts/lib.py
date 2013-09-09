
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
   sys.stdout.write("!edge(@" + str(a) + ",@" + str(b) + "," + str(w) + ").\n")

def write_dedgew(a, b, w):
	write_edgew(a, b, w)
	write_edgew(b, a, w)

def simple_write_edge(a, b):
	sys.stdout.write("!edge(@" + str(a) + ",@" + str(b) + ").\n")

def write_inout(a, b):
	sys.stdout.write("!output(@" + str(a) + ", @" + str(b) + "). ")
	sys.stdout.write("!input(@" + str(b) + ", @" + str(a) + ").\n")

def write_winout(a, b, w):
	print "!output(@" + str(a) + ", @" + str(b) + ", " + str("%.7f" % w) + ")."
	print "!input(@" + str(b) + ", @" + str(a) + ", " + str("%.7f" % w) + ")."
