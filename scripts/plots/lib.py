
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
            '13queens': "13 Queens",
            'tree': "Tree",
            'tree-coord': "Tree (coordinated)",
            'min-max-tictactoe': "MiniMax",
            'min-max-tictactoe-coord': "MiniMax (coordinated)",
            'belief-propagation-200': "Belief Propagation (200x200)",
            'belief-propagation-400': "Belief Propagation (400x400)",
            'splash-bp-400': "Belief Propagation (400x400 with Splashes)",
            'graphlab': "GraphLab Belief Propagation (400x400)",
            'pagerank-5000': "PageRank (5000 nodes)",
            'pagerank-search_engines': "PageRank (Search Engines)",
            'new-heat-transfer-120': "Heat Transfer (120x120)",
            'new-heat-transfer-80': "Heat Transfer (80x80)"}
   return table[name]

def coordinated_program(name):
   if name == 'belief-propagation-400':
      return 'splash-bp-400'
   return name + '-coord'

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

      cmap = plt.get_cmap('gray')
      otherRect = ax.bar(ind, [1] * n, width, color=cmap(0.2))
      timesNew = list(x.get_time(threads) for x in self.experiments.values())
      timesOld = list(x.get_time(threads) for x in other.experiments.values())

      thisRect = ax.bar(ind + width, map(truediv, timesNew, timesOld), width, color=cmap(0.7))

      ax.set_ylabel('Relative Execution')
      title = 'Comparison (' + str(threads) + ' thread'
      if threads > 1:
         title += 's'
      title += ')'
      ax.set_title(title)
      ax.set_xticks(ind+width)
      ax.set_xticklabels(list(name2title(x) for x in self.experiments.keys()), rotation=45, fontsize=10, ha='right')
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
      return [0] + self.x_axis1()

   def x_axis1(self):
      return [key for key in self.times.keys() if key <= max_threads]

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

   def get_improvement(self, reg):
      return [float(reg.get_time(th))/float(c) for th, c in self.times.iteritems() if th <= max_threads]

   def create_coord_improv(self, prefix, coordinated):
      fig = plt.figure()
      ax = fig.add_subplot(111)

      names = ('Improvement')
      formats = ('f4')
      titlefontsize = 22
      ylabelfontsize = 20
      ax.set_title(self.title, fontsize=titlefontsize)
      ax.yaxis.tick_right()
      ax.yaxis.set_label_position("right")
      ax.set_ylabel('Improvement', fontsize=ylabelfontsize)
      ax.set_xlabel('Threads', fontsize=ylabelfontsize)
      max_value_threads = max(x for x in self.times.keys() if x <= max_threads)
      improvs = coordinated.get_improvement(self)
      ax.set_xlim([1, max_value_threads])
      ax.set_ylim([0, math.ceil(max(improvs))])

      cmap = plt.get_cmap('gray')

      ax.plot(self.x_axis1(), improvs,
         label='Coordination', linestyle='--', marker='^', color=cmap(0.1))
# ax.legend([reg, coord], ["Regular", "Coordinated"], loc=2, fontsize=20, markerscale=2)

      setup_lines(ax, cmap)

      name = prefix + self.name + ".png"
      plt.savefig(name)

   def create_ht(self, coord, localonly):
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
      mspeedup = max(self.max_speedup(), localonly.max_speedup(self.get_time(1)))
      ax.set_xlim([0, max_value_threads])
      ax.set_ylim([0, mspeedup])

      cmap = plt.get_cmap('gray')

      reg, = ax.plot(self.x_axis(), self.speedup_data(),
         linestyle='--', marker='^', color=cmap(0.1))
      coordcoord, = ax.plot(self.x_axis(), coord.speedup_data(),
         linestyle='--', marker='s', color=cmap(0.4))
      coordreg, = ax.plot(self.x_axis(), coord.speedup_data(self.get_time(1)),
         linestyle='--', marker='o', color=cmap(0.4))
      loreg, = ax.plot(self.x_axis(), localonly.speedup_data(self.get_time(1)),
         linestyle='--', marker='D', color=cmap(0.4))
      ax.plot(self.x_axis(), self.linear_speedup(),
        label='Linear', linestyle='-', color=cmap(0.2))
      ax.legend([reg, coordcoord, coordreg, loreg], ["Regular/Regular",
            "Coordinated/Coordinated", "Coordinated/Regular", "Local-Only/Regular"], loc=2, fontsize=18, markerscale=2)

      setup_lines(ax, cmap)

      name = self.name + ".png"
      plt.savefig(name)

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
      ax.set_xlim([0, max_value_threads])
      ax.set_ylim([0, mspeedup])

      cmap = plt.get_cmap('gray')

      reg, = ax.plot(self.x_axis(), self.speedup_data(),
         linestyle='--', marker='^', color=cmap(0.1))
      coordcoord, = ax.plot(self.x_axis(), coordinated.speedup_data(),
         linestyle='--', marker='s', color=cmap(0.4))
      coordreg, = ax.plot(self.x_axis(), coordinated.speedup_data(self.get_time(1)),
         linestyle='--', marker='o', color=cmap(0.4))
      ax.plot(self.x_axis(), self.linear_speedup(),
        label='Linear', linestyle='-', color=cmap(0.2))
      ax.legend([reg, coordcoord, coordreg], ["Regular/Regular",
            "Coordinated/Coordinated", "Coordinated/Regular"], loc=2, fontsize=20, markerscale=2)

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

   def create_comparison_coord_system(self, prefix, coord, system, coord_system, system_name='GraphLab', title=None):
      fig = plt.figure()
      ax = fig.add_subplot(111)

      names = ('Speedup')
      formats = ('f4')
      titlefontsize = 22
      ylabelfontsize = 20
      if not title:
         title = "Coordination Improvement"
      ax.set_title(title, fontsize=titlefontsize)
      ax.yaxis.tick_right()
      ax.yaxis.set_label_position("right")
      ax.set_ylabel('Improvement', fontsize=ylabelfontsize)
      ax.set_xlabel('Threads', fontsize=ylabelfontsize)
      max_value_threads = max(x for x in self.times.keys() if x <= max_threads)
      improv_lm = coord.get_improvement(self)
      improv_system = coord_system.get_improvement(system)
      ax.set_xlim([1, max_value_threads])
      ax.set_ylim([0, math.ceil(max(max(improv_lm), max(improv_system))) + 1])

      cmap = plt.get_cmap('gray')

      lm, = ax.plot(self.x_axis1(), improv_lm,
         label='CLM Improvement', linestyle='--', marker='^', color=cmap(0.1))
      other, = ax.plot(self.x_axis1(), improv_system,
         label=system_name + ' Improvement', linestyle='--', marker='+', color=cmap(0.6))
      ax.legend([lm, other], ["CLM", system_name], loc=2, fontsize=20, markerscale=2)

      setup_lines(ax, cmap)

      name = prefix + "improve_" + self.name + ".png"
      plt.savefig(name)


   def create_speedup_compare_systems(self, prefix, system, system_name='GraphLab', title=None):
      fig = plt.figure()
      ax = fig.add_subplot(111)

      names = ('Speedup')
      formats = ('f4')
      titlefontsize = 22
      ylabelfontsize = 20
      if not title:
         title = self.title
      ax.set_title(title, fontsize=titlefontsize)
      ax.yaxis.tick_right()
      ax.yaxis.set_label_position("right")
      ax.set_ylabel('Speedup', fontsize=ylabelfontsize)
      ax.set_xlabel('Threads', fontsize=ylabelfontsize)
      max_value_threads = max(x for x in self.times.keys() if x <= max_threads)
      ax.set_xlim([0, max_value_threads])
      ax.set_ylim([0, max(self.max_speedup(), system.max_speedup())])

      cmap = plt.get_cmap('gray')

      lm, = ax.plot(self.x_axis(), self.speedup_data(),
         label='LM Speedup', linestyle='--', marker='^', color=cmap(0.1))
      other, = ax.plot(self.x_axis(), system.speedup_data(),
         label=system_name + ' Speedup', linestyle='--', marker='+', color=cmap(0.6))
      ax.plot(self.x_axis(), self.linear_speedup(),
        label='Linear', linestyle='-', color=cmap(0.2))
      ax.legend([lm, other], ["CLM", system_name], loc=2, fontsize=20, markerscale=2)

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
      try:
         if name.endswith("-coord"):
            self.title = name2title(name[:-len("-coord")])
         else:
            self.title = name2title(name)
      except KeyError:
         self.title = "(Not found)"
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

