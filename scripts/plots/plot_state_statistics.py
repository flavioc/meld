#!/usr/bin/python

import sys
import os
import math
import csv
import matplotlib.pyplot as plt
import matplotlib as mpl
import numpy as np
import scipy
from scipy.interpolate import spline
from matplotlib import rcParams
from numpy import nanmax
from lib import name2title, experiment_set, experiment, coordinated_program

if len(sys.argv) != 2:
   print "plot_state_statistics.py <dir>"
   sys.exit(1)


def to_int(val):
   try:
      return int(val)
   except ValueError:
      return 0

class instrumentation_slice(object):
   def compute_average(self):
      length = len(self.data)
      assert(length > 0)
      total = sum(to_int(x) for x in self.data)
      return float(total) / float(length)

   def compute_median(self):
      int_data = sorted([to_int(x) for x in self.data])
      length = len(int_data)
      assert(length > 0)
      index = (length - 1) // 2
      if length % 2:
         return int_data[index]
      return (int_data[index] + int_data[index + 1])/2.0

   def __init__(self, data):
      self.data = data


class instrumentation_item(object):
   def add_slice(self, time, slc):
      self.slices[int(time)] = slc

   def ordered_average(self):
      return [self.slices[time].compute_average() for time in sorted(self.slices)]

   def ordered_median(self):
      return [self.slices[time].compute_median() for time in sorted(self.slices)]

   def time(self): return [time for time in sorted(self.slices)]

   def get_num_threads(self): return self.threads

   def __init__(self, threads):
      self.slices = {}
      self.threads = threads


class instrumentation_state(instrumentation_item):

   def __init__(self, threads):
      super(instrumentation_state, self).__init__(threads)


class instrumentation_benchmark(object):
   def get_name(self): return self.name

   def add_item(self, name, thread, item):
      self.infos[(name, int(thread))] = item

   def get_item(self, name, thread): return self.infos[(name, thread)]

   def __init__(self, name):
      self.name = name
      self.infos = {}


class instrumentation_set(object):
   def add_benchmark(self, ins):
      self.benchs[ins.get_name()] = ins

   def __iter__(self):
      return iter(self.benchs.values())

   def __init__(self):
      self.benchs = {}


def parse_state(item, path):
   with open(path, 'rb') as csvfile:
      reader = csv.reader(csvfile, delimiter=' ')
      next(reader)
      for row in reader:
         time = int(row[0])
         slc = instrumentation_slice(row[1:])
         item.add_slice(time, slc)


def parse_instrumentation_benchmark(subdir, name):
   bench = instrumentation_benchmark(name)
   for th in os.listdir(subdir):
      path = os.path.join(subdir, th)
      state = os.path.join(path, "data.state")
      if not os.path.isfile(state):
         print "**WARNING** file missing", state
         continue
      item = instrumentation_state(th)
      parse_state(item, state)
      bench.add_item("state", th, item)
   return bench


instrument_dir = sys.argv[1]
ins_set = instrumentation_set()
for name in os.listdir(instrument_dir):
   subdir = os.path.join(instrument_dir, name)
   if os.path.isdir(subdir):
      print "Processing", name
      bench = parse_instrumentation_benchmark(subdir, name)
      ins_set.add_benchmark(bench)


def runningMeanFast(x, N):
   return np.convolve(x, np.ones((N,))/N)[(N-1):]


def plot_state_evolution(th, data, average):
   fig = plt.figure(figsize = (10,4))
   ax = fig.add_subplot(111)

   titlefontsize = 22
   ylabelfontsize = 20
   ax.set_title(str(th) + " threads", fontsize=titlefontsize)
   ax.yaxis.tick_right()
   ax.yaxis.set_label_position("right")
   ax.set_ylabel('Average', fontsize=ylabelfontsize)
   ax.set_xlabel('Time', fontsize=ylabelfontsize)

   cmap = plt.get_cmap('gray')

   plots = []
   length = len(data)
   inc = 0.5 / float(length+1)
   i = 0
   linestyles = ['_', '-', '--', ':']
   colors=['r', 'b', 'g', 'brown', 'black']
   for (name, state) in data:
      i = i + 1
      x = np.array(state.time())
      if average:
         y0 = state.ordered_average()
      else:
         y0 = state.ordered_median()
      y = runningMeanFast(y0, 100)
      plot, = ax.plot(x, y,
         linestyle='-', color=colors[i % len(colors)])
      plots.append(plot)
   ax.legend(plots, [name for (name, _) in data], loc=1, fontsize=12, markerscale=2)
   if average:
      name = "average.png"
   else:
      name = "median.png"
   name = "th" + str(th) + "-" + name
   print "Generating", name
   plt.savefig(name)

for th in [1, 2, 4, 6, 8, 10, 12, 14, 16]:
   data = []
   failed = False
   for bench in ins_set:
      try:
         state = bench.get_item("state", th)
         data.append((bench.get_name(), state))
      except KeyError:
         failed = True
         print "**WARNING** missing"
   if not failed:
      plot_state_evolution(th, data, True)
      plot_state_evolution(th, data, False)

