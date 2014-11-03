#!/usr/bin/python

import sys
import math
from lib import name2title, experiment_set, experiment, coordinated_program

if len(sys.argv) != 3:
  print "Usage: plot_compare_system.py <filename> <output_prefix>"
  sys.exit(1)

filename = sys.argv[1]
prefix = sys.argv[2]

# read data.
expset = experiment_set()
with open(filename, "r") as fp:
   for line in fp:
      line = line.rstrip("\n")
      if line == "":
         continue
      vec = line.split(" ")
      if len(vec) < 3:
         continue
      name = vec[0]
      spec = vec[1]
      time = int(vec[len(vec)-1])
      threads = int(spec[2:])
      expset.add_experiment(name, threads, time)

def name2othersystem(name):
   if name == 'belief-propagation-400':
      return 'graphlab'
   if name == 'splash-bp-400':
      return 'graphlab-coord'
   return None

# Generate the speedup graphs.
for exp_name in expset.experiment_names():
   lm = expset.get_experiment(exp_name)
   othersystem = name2othersystem(exp_name)
   if not othersystem:
      continue
   other = expset.get_experiment(othersystem)
   if not other:
      continue
   lm.create_speedup_compare_systems(prefix, other, 'GraphLab')
   coord_lm = expset.get_experiment(coordinated_program(exp_name))
   if not coord_lm:
      continue
   coord_other = expset.get_experiment(coordinated_program(othersystem))
   if not coord_other:
      continue
   lm.create_comparison_coord_system(prefix, other, coord_lm, coord_other, 'GraphLab')
