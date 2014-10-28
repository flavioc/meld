
import math
import matplotlib.pyplot as plt
import matplotlib as mpl
import numpy as np
from operator import truediv
from matplotlib import rcParams
from numpy import nanmax

max_threads = 16

def name2title(name):
   table = {'greedy-graph-coloring-2000': "Greedy Graph Coloring (2000 nodes)",
            'greedy-graph-coloring-search_engines': "Greedy Graph Coloring (Search Engines)",
            'shortest-uspowergrid': "Shortest Distance (US Power Grid)",
            'shortest-usairports500': "Shortest Distance (US 500 Airports)",
            'shortest-celegans': "Shortest Distance (Celegans)",
            'shortest-1000': "Shortest Distance (1000)",
            'shortest-oclinks': "Shortest Distance (OCLinks)",
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
      try:
         return self.experiments[name]
      except KeyError:
         return None

   def create_histogram_compare(self, prefix, other, threads):
      n = len(self.experiments)
      ind = np.arange(n)
      width = 0.35

      fig, ax = plt.subplots()

      otherRect = ax.bar(ind, [1] * n, width, color='r')
      timesNew = list(x.get_time(threads) for x in self.experiments.values())
      timesOld = list(x.get_time(threads) for x in other.experiments.values())

      thisRect = ax.bar(ind + width, map(truediv, timesNew, timesOld), width, color='y')

      ax.set_ylabel('Relative Execution')
      title = 'Comparison (' + str(threads) + ' thread'
      if threads > 1:
         title += 's'
      title += ')'
      ax.set_title(title)
      ax.set_xticks(ind+width)
      ax.set_xticklabels(list(name2title(x) for x in self.experiments.keys()), rotation=45, fontsize=8, ha='right')
      ax.legend((otherRect[0], thisRect[0]), ('Old', 'New'))
      ax.set_ylim([0, 1.5])

      plt.tight_layout()
      name = prefix + "comparison" + str(threads) + ".png"
      plt.savefig(name)

   def __init__(self):
      self.experiments = {}

class experiment(object):

   def add_time(self, nthreads, time):
      self.times[nthreads] = time

   def get_time(self, nthreads):
      return float(self.times[nthreads])

   def x_axis(self):
      return [0] + [key for key in self.times.keys() if key <= max_threads]

   def speedup_data(self, base=None):
      if not base:
         base = self.times[1]
      return [None] + [float(base)/float(x) for th, x in self.times.iteritems() if th <= max_threads]

   def linear_speedup(self):
      return self.x_axis()

   def max_speedup(self, base=None):
      if not base:
         base = self.times[1]
      m = max(float(base)/float(x) for th, x in self.times.iteritems() if th <= max_threads)
      if m <= 16:
         return 16
      else:
         return math.ceil(m) + 2

   def create_coord(self, prefix, coordinated):
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
      mspeedup = max(self.max_speedup(), coordinated.max_speedup(self.get_time(1)))
      print mspeedup
      ax.set_xlim([0, max_value_threads])
      ax.set_ylim([0, mspeedup])

      cmap = plt.get_cmap('gray')

      reg, = ax.plot(self.x_axis(), self.speedup_data(),
         label='Speedup (Regular)', linestyle='--', marker='^', color=cmap(0.1))
      coord, = ax.plot(self.x_axis(), coordinated.speedup_data(self.get_time(1)),
         label='Speedup (Coordinated)', linestyle='--', marker='o', color=cmap(0.4))
      ax.plot(self.x_axis(), self.linear_speedup(),
        label='Linear', linestyle='-', color=cmap(0.2))
      ax.legend([reg, coord], ["Regular", "Coordinated"], loc=2, fontsize=20, markerscale=2)

      setup_lines(ax, cmap)

      name = prefix + self.name + ".png"
      plt.savefig(name)

   def create_speedup(self, prefix):
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

   def create_speedup_compare(self, prefix, other):
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

      new, = ax.plot(self.x_axis(), self.speedup_data(),
         label='New Speedup', linestyle='--', marker='^', color=cmap(0.1))
      old, = ax.plot(self.x_axis(), other.speedup_data(),
         label='Old Speedup', linestyle='--', marker='+', color=cmap(0.6))
      ax.plot(self.x_axis(), self.linear_speedup(),
        label='Linear', linestyle='-', color=cmap(0.2))
      ax.legend([new, old], ["New Version", "Old Version"], loc=2, fontsize=20, markerscale=2)

      setup_lines(ax, cmap)

      name = prefix + self.name + ".png"
      plt.savefig(name)

   def __init__(self, name):
      self.name = name
      if name.endswith("-coord"):
         self.title = name2title(name[:-len("-coord")])
      else:
         self.title = name2title(name)
      self.times = {}


def setup_lines(ax, cmap):
   lines = ax.lines
   markersize = 20
   markerspace = 1
   c = cmap(0.5)

   for i, ln in enumerate(lines):
     ln.set_linewidth(4)
     ln.set_markersize(markersize)
     ln.set_markevery(markerspace)

