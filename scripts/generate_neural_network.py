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

print "type route link(node, node)."
print "type input(node)."
print "type hidden(node)."
print "type output(node)."
print "type bias(node)."
print "type weight(node, int, node, float)."
print

for neuron in range(num_input):
   print "input(@" + str(neuron + INPUT_BASE) + ").",
print
for neuron in range(num_hidden):
   print "hidden(@" + str(neuron + HIDDEN_BASE) + ").",
print
for neuron in range(num_output):
   print "output(@" + str(neuron + OUTPUT_BASE) + ").",
print
print

# create input bias
bias_input = str(INPUT_BASE + num_input)
print "input(@" + bias_input + ")."
print "bias(@" + bias_input + ")."

# create hidden bias
bias_hidden = str(HIDDEN_BASE + num_hidden)
print "hidden(@" + bias_hidden + ")."
print "bias(@" + bias_hidden + ")."
print

# link every input neuron (including input bias) to every hidden neuron (excluding bias)
for i in range(num_input + 1):
   for h in range(num_hidden):
      print "link(@" + str(i + INPUT_BASE) + ", @" + str(h + HIDDEN_BASE) + ").",
   print
print

# link every hidden neuron (including hidden bias) to every output neuron
for h in range(num_hidden + 1):
   for o in range(num_output):
      print "link(@" + str(h + HIDDEN_BASE) + ", @" + str(o + OUTPUT_BASE) + ").",
   print
print
   
# generate weights for links from input to hidden neurons
for i in range(num_input + 1):
   for h in range(num_hidden):
      print "weight(@" + str(i + INPUT_BASE) + ", 0, @" + str(h + HIDDEN_BASE) + ", 0.5).",
   print
print

# generate weights for links from hidden to output neurons
for h in range(num_hidden + 1):
   for o in range(num_output):
      print "weight(@" + str(h + HIDDEN_BASE) + ", 0, @" + str(o + OUTPUT_BASE) + ", 0.5).",
   print
print

if len(sys.argv) == 6:
	MAX_READ = int(sys.argv[5])
else:
	MAX_READ = 500

reader = csv.reader(open(file, 'rb'), delimiter=',')
count = 0
for row in reader:
   for i in range(num_input):
      val = row[i]
      print "activated(@" + str(i + INPUT_BASE) + ", " + str(count) + ", " + str(val) + ").",
   print
   for o in range(num_output):
      val = row[num_input + o]
      print "expected(@" + str(o + OUTPUT_BASE) + ", " + str(count) + ", " + str(val) + ").",
   print
   # generate bias node value
   print "activated(@" + bias_input + ", " + str(count) + ", 0.0 - 1.0)."
   print "activated(@" + bias_hidden + ", " + str(count) + ", 0.0 - 1.0)."
   count = count + 1
   if count == MAX_READ:
      sys.exit(1)
