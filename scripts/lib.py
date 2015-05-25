
import sys
import random

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

def write_list(ls):
    s = "["
    first = True
    for x in ls:
        if first:
            first = False
        else:
           s = s + ", "
        s = s + str(x)
    return s + "]"

def list_has(list, x):
    try:
        list.index(x)
        return True
    except ValueError:
        return False

def generate_graph(total, directed=False):
    if total < 200:
        factor = 0.5
    else:
        factor = 1.0/(total/20.0)
    nodes = {}
    for i in range(total):
        links = random.randint(1, int(total*factor))
        list = []
        for j in range(links):
            link = random.randint(1, total - 1)
            if link == i:
                link = (link + 1) % total
            if directed:
               try:
                  links_other = nodes[link]
                  if list_has(links_other, i):
                     continue
               except KeyError:
                  pass
            if not list_has(list, link):
                list.append(link)
        nodes[i] = list
    return nodes
