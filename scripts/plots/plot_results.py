#!/usr/bin/python

import sys
import math
import matplotlib.pyplot as plt
import matplotlib as mpl
import numpy as np
from matplotlib import rcParams
from numpy import nanmax

if len(sys.argv) != 3:
  print "Usage: plot.py <filename> <output_prefix>"
  sys.exit(1)

filename = sys.argv[1]
prefix = sys.argv[2]
max_threads = 16

def setup_lines(ax, cmap):
   lines = ax.lines
   markersize = 20
   markerspace = 1
   c = cmap(0.5)

   for i, ln in enumerate(lines):
     ln.set_linewidth(4)
     ln.set_markersize(markersize)
     ln.set_markevery(markerspace)

def name2title(name):
   table = {'greedy-graph-coloring-2000': "Greedy Graph Coloring (2000 nodes)",
            'greedy-graph-coloring-search_engines': "Greedy Graph Coloring (Search Engines)",
            'shortest-uspowergrid': "Shortest Distance (US Power Grid)",
            'shortest-usairports500': "Shortest Distance (US 500 Airports)",
            '8queens-13': "13 Queens",
            'tree': "Tree",
            'tree-coord': "Tree (coordinated)",
            'min-max-tictactoe': "MiniMax",
            'min-max-tictactoe-coord': "MiniMax (coordinated)",
            'belief-propagation-200': "Belief Propagation (200x200)",
            'belief-propagation-400': "Belief Propagation (400x400)",
            'pagerank-5000': "PageRank (5000 nodes)",
            'pagerank-search_engines': "PageRank (Search Engines)",
            'new-heat-transfer-80': "Heat Transfer (80x80)"}
   return table[name]

class experiment(object):

   def add_time(self, nthreads, time):
      self.times[nthreads] = time

   def x_axis(self):
      return [0] + [key for key in self.times.keys() if key <= max_threads]

   def speedup_data(self):
      base = self.times[1]
      return [None] + [float(base)/float(x) for th, x in self.times.iteritems() if th <= max_threads]

   def linear_speedup(self):
      return self.x_axis()

   def max_speedup(self):
      base = self.times[1]
      m = max(float(base)/float(x) for th, x in self.times.iteritems() if th <= max_threads)
      if m <= 16:
         return 16
      else:
         return math.ceil(m) + 2

   def create_speedup(self):
      fig = plt.figure()
      ax = fig.add_subplot(111)

      names = ('Speedup')
      formats = ('f4')
      titlefontsize = 22
      ylabelfontsize = 20
      ax.set_title(self.title, fontsize=titlefontsize)
      ax.yaxis.tick_right()
      ax.yaxis.set_label_position("right")
      ax.set_ylabel('Speedup', fontsize=ylabelfontsize)
      ax.set_xlabel('Threads', fontsize=ylabelfontsize)
      max_value_threads = max(x for x in self.times.keys() if x <= max_threads)
      ax.set_xlim([0, max_value_threads])
      ax.set_ylim([0, self.max_speedup()])

      cmap = plt.get_cmap('gray')

      ax.plot(self.x_axis(), self.speedup_data(),
         label='Speedup', linestyle='--', marker='^', color=cmap(0.1))
      ax.plot(self.x_axis(), self.linear_speedup(),
        label='Linear', linestyle='-', color=cmap(0.2))

      setup_lines(ax, cmap)

      name = prefix + self.name + ".png"
      plt.savefig(name)


   def __init__(self, name):
      self.name = name
      self.title = name2title(name)
      self.times = {}

class experiment_set(object):
   def add_experiment(self, name, threads, time):
      try:
         exp = self.experiments[name]
         exp.add_time(threads, time)
      except KeyError:
         exp = experiment(name)
         self.experiments[name] = exp
         exp.add_time(threads, time)

   def experiment_names(self):
      return self.experiments.keys()

   def get_experiment(self, name):
      return self.experiments[name]

   def __init__(self):
      self.experiments = {}

# read data.
expset = experiment_set()
with open(filename, "r") as fp:
   for line in fp:
      line = line.rstrip("\n")
      if line == "":
         continue
      vec = line.split(" ")
      if len(vec) < 5:
         continue
      name = vec[0]
      threads = int(vec[1][2:])
      time = int(vec[len(vec)-1])
      expset.add_experiment(name, threads, time)

for exp_name in expset.experiment_names():
   exp = expset.get_experiment(exp_name)
   exp.create_speedup()
