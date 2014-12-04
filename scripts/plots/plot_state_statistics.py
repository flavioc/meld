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

if len(sys.argv) != 3:
   print "plot_state_statistics.py <dir> <prefix>"
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


def parse_numbers(item, path):
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
      state_file = os.path.join(path, "data.state")
      if os.path.isfile(state_file):
         state = instrumentation_item(th)
         parse_numbers(state, state_file)
         bench.add_item("state", th, state)
      else:
         print "**WARNING** file", state_file, "is missing"
      derived_facts_file = os.path.join(path, "data.derived_facts")
      if os.path.isfile(derived_facts_file):
         derived_facts = instrumentation_item(th)
         parse_numbers(derived_facts, derived_facts_file)
         bench.add_item("derived_facts", th, derived_facts)
      else:
         print "**WARNING** file", derived_facts_file, "is missing"
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


def plot_numeric_evolution(title, th, data, average=True, mean=100):
   fig = plt.figure(figsize = (10,4))
   ax = fig.add_subplot(111)

   titlefontsize = 22
   ylabelfontsize = 20
   ax.set_title(title + " / " + str(th) + " threads", fontsize=titlefontsize)
   ax.yaxis.tick_right()
   ax.yaxis.set_label_position("right")
   if average:
      ax.set_ylabel('Average', fontsize=ylabelfontsize)
   else:
      ax.set_ylabel('Median', fontsize=ylabelfontsize)
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
      y = runningMeanFast(y0, mean)
      plot, = ax.plot(x, y,
         linestyle='-', color=colors[i % len(colors)])
      plots.append(plot)
   ax.legend(plots, [name for (name, _) in data], loc=1, fontsize=12, markerscale=2)
   if average:
      end_file = "average.png"
   else:
      end_file = "median.png"
   file_name = sys.argv[2] + "_" + title + "_th" + str(th) + "-" + end_file
   print "Generating", file_name
   plt.savefig(file_name)

for th in [1, 2, 4, 6, 8, 10, 12, 14, 16]:
   state_data = []
   derived_facts_data = []
   failed = False
   for bench in ins_set:
      try:
         state = bench.get_item("state", th)
         state_data.append((bench.get_name(), state))
         derived_facts = bench.get_item("derived_facts", th)
         derived_facts_data.append((bench.get_name(), derived_facts))
      except KeyError:
         failed = True
   if not failed:
      plot_numeric_evolution("nodes", th, state_data) # average
      plot_numeric_evolution("derived_facts", th, derived_facts_data, True, 250) # average
      

