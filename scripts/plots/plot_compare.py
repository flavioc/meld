#!/usr/bin/python

import sys
import math
from lib import name2title, experiment_set, experiment

if len(sys.argv) < 3:
  print "Usage: plot_compare.py <filename> <output_prefix> [--global-only]"
  sys.exit(1)

filename = sys.argv[1]
prefix = sys.argv[2]

# read data.
new = experiment_set()
old = experiment_set()
with open(filename, "r") as fp:
   for line in fp:
      line = line.rstrip("\n")
      if line == "":
         continue
      vec = line.split(" ")
      if len(vec) < 5:
         continue
      name = vec[0]
      spec = vec[1]
      time = int(vec[len(vec)-1])
      if spec.startswith("th"):
         threads = int(spec[2:])
         new.add_experiment(name, threads, time)
      else:
         threads = int(spec[1:])
         old.add_experiment(name, threads, time)

if len(sys.argv) == 3:
   # Generate the speedup graphs.
   for exp_name in new.experiment_names():
      newexp = new.get_experiment(exp_name)
      oldexp = old.get_experiment(exp_name)
      newexp.create_speedup_compare(prefix, oldexp)

# Generate the histogram.
new.create_histogram_compare(prefix, old, 1)
new.create_histogram_compare(prefix, old, 16)
