#!/usr/bin/python

import sys
import math
import matplotlib.pyplot as plt
import matplotlib as mpl
import numpy as np
from matplotlib import rcParams
from numpy import nanmax
from lib import name2title, experiment_set, experiment, coordinated_program

if len(sys.argv) != 3:
  print "Usage: plot_coord.py <filename> <output_prefix>"
  sys.exit(1)

filename = sys.argv[1]
prefix = sys.argv[2]
max_threads = 16

# read data.
expset = experiment_set()
with open(filename, "r") as fp:
   for line in fp:
      line = line.rstrip("\n")
      if line == "":
         continue
      if line.startswith("#"):
         continue
      vec = line.split(" ")
      if len(vec) < 3:
         continue
      name = vec[0]
      threads = int(vec[1][2:])
      time = int(vec[len(vec)-1])
      expset.add_experiment(name, threads, time)

for exp_name in expset.experiment_names():
   if exp_name.endswith("-coord"):
      continue
   coord = expset.get_experiment(coordinated_program(exp_name))
   if not coord:
      continue
   exp = expset.get_experiment(exp_name)
   exp.create_coord(prefix, coord)
