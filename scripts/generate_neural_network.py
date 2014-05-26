#!/usr/bin/python

INPUT_BASE = 0
HIDDEN_BASE = 100
OUTPUT_BASE = 200

import sys
import csv

if len(sys.argv) < 5:
	print "Usage: generate_neural_network.py <input neurons> <hidden neurons> <output neurons> <file> [max data]"
	sys.exit(1)

num_input = int(sys.argv[1])
num_hidden = int(sys.argv[2])
num_output = int(sys.argv[3])

if num_input >= 100 or num_hidden >= 100:
	print "num_input or num_hidden can't be greater than 100"
	sys.exit(1)

file = sys.argv[4]
print_header = False

if print_header:
   print "type route link(node, node)."
   print "type route back-link(node, node)."
   print "type input(node)."
   print "type hidden(node)."
   print "type output(node)."
   print "type bias(node)."
   print "type linear weight(node, int, node, float)."
   print

def div_ceil(a, b):
   if a % b:
      return ((a/b)+1)
   else:
      return (a/b)

input_nodes = []
input_bias_node = 0
hidden_nodes = []
hidden_bias_node = 0
output_nodes = []
current_node = 0
take_input = 0
take_hidden = 0
take_output = 0
total_input = 0
total_hidden = 0
total_output = 0
if num_hidden >= num_input and num_input >= num_output:
   print "error"
   sys.exit(1)
   # hidden > input > output
elif num_output >= num_hidden and num_hidden >= num_input:
   # output > hidden > input
   take_hidden = 1
   take_output = num_output / num_hidden
   take_input = num_hidden / num_input
   print "error"
   sys.exit(1)
elif num_input >= num_hidden and num_hidden >= num_output:
   # input > hidden > output
   take_input = div_ceil(num_input + 1, num_hidden + 1)
   take_hidden = 1
   take_output = (num_hidden+1)/(num_output+1)
   for x in range(num_hidden + 1):
      for i in range(take_input):
         if total_input < num_input + 1:
            total_input = total_input + 1
            input_nodes.append(current_node)
            input_bias_node = current_node # last
            current_node = current_node + 1
      hidden_nodes.append(current_node)
      hidden_bias_node = current_node # last
      current_node = current_node + 1
      total_hidden = total_hidden + 1
      if x % take_output == 0 and total_output < num_output:
         total_output = total_output + 1
         output_nodes.append(current_node)
         current_node = current_node + 1

#print input_nodes
#print hidden_nodes
#print output_nodes

for neuron in input_nodes:
   print "!input(@" + str(neuron) + ").",
   print "start(@" + str(neuron) + ", 0).",
print
for neuron in hidden_nodes:
   print "!hidden(@" + str(neuron) + ").",
print
for neuron in output_nodes:
   print "!output(@" + str(neuron) + ").",
print
print

# create input bias
print "!bias(@" + str(input_bias_node) + ")."

# create hidden bias
print "!bias(@" + str(hidden_bias_node) + ")."
print "start(@" + str(hidden_bias_node) + ", 0)."
print

def print_link(id1, id2):
	print "!link(@" + str(id1) + ", @" + str(id2) + ")."
	print "!back-link(@" + str(id2) + ", @" + str(id1) + ")."

# link every input neuron (including input bias) to every hidden neuron (excluding bias)
for i in input_nodes:
   for h in hidden_nodes[:-1]:
		print_link(i, h)
   print
print

# link every hidden neuron (including hidden bias) to every output neuron
for h in hidden_nodes:
   for o in output_nodes:
		print_link(h, o)
   print
print
   
# generate weights for links from input to hidden neurons
for i in input_nodes:
   for h in hidden_nodes[:-1]:
      print "weight(@" + str(i) + ", 0, @" + str(h) + ", 0.5).",
   print
   print "!totaloutput(@" + str(i) + ", " + str(num_hidden) + ")."
   print "missing-gradients(@" + str(i) + ", " + str(num_hidden) + ")."
print

# generate weights for links from hidden to output neurons
for h in hidden_nodes:
   for o in output_nodes:
      print "weight(@" + str(h) + ", 0, @" + str(o) + ", 0.5).",
   if h == num_hidden:
      pass
   else:
      print "!totalinput(@" + str(h) + ", " + str(num_input + 1) + ")."
      print "toreceive(@" + str(h) + ", " + str(num_input + 1) + ", 0, 0.0)."
   print
print

for o in output_nodes:
   print "!totalinput(@" + str(o) + ", " + str(num_hidden + 1) + ")."
   print "toreceive(@" + str(o) + ", " + str(num_hidden + 1) + ", 0, 0.0)."

if len(sys.argv) == 6:
	MAX_READ = int(sys.argv[5])
else:
	MAX_READ = 500

reader = csv.reader(open(file, 'rb'), delimiter=',')
count = 0
for row in reader:
   idx = 0
   for i in input_nodes[:-1]:
      val = row[idx]
      idx = idx + 1
      print "activated(@" + str(i) + ", " + str(count) + ", " + str(val) + ").",
   print
   for o in output_nodes:
      val = row[idx]
      idx = idx + 1
      print "expected(@" + str(o) + ", " + str(count) + ", " + str(val) + ").",
   print
   # generate bias node value
   print "activated(@" + str(input_bias_node) + ", " + str(count) + ", 0.0 - 1.0)."
   count = count + 1
   if count == MAX_READ:
      sys.exit(1)
