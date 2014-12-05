#!/usr/bin/python

import sys
import os
import math
import csv
import matplotlib.pyplot as plt
import matplotlib as mpl
import numpy as np
import scipy
import itertools
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
      total = sum(x for x in self.data)
      return float(total) / float(length)

   def compute_median(self):
      int_data = sorted([x for x in self.data])
      length = len(int_data)
      assert(length > 0)
      index = (length - 1) // 2
      if length % 2:
         return int_data[index]
      return (int_data[index] + int_data[index + 1])/2.0

   def add(self, other):
      self.data = [a + b for a, b in itertools.izip(self.data, other.data)]

   def sub(self, other):
      self.data = [a - b for a, b in itertools.izip(self.data, other.data)]

   def create_zeros(self):
      return instrumentation_slice([0 for x in self.data])

   def __init__(self, data):
      self.data = [to_int(x) for x in data]


def add_slices(ls):
   f = ls[0]
   s = f.create_zeros()
   for x in ls:
      s.add(x)
   return s


class instrumentation_item(object):
   def add_slice(self, time, slc):
      self.slices[int(time)] = slc

   def add(self, otheritem):
      for time, sl in self.slices.iteritems():
         sl.add(otheritem.slices[time])

   def sub(self, otheritem):
      for time, sl in self.slices.iteritems():
         sl.sub(otheritem.slices[time])

   def ordered_average(self):
      return [self.slices[time].compute_average() for time in sorted(self.slices)]

   def ordered_median(self):
      return [self.slices[time].compute_median() for time in sorted(self.slices)]

   def apply_running_total(self, window=5):
      sum_list = []
      included = 0
      new_slices = {}
      for time, slic in self.slices.iteritems():
         if included < window:
            sum_list.append(slic)
            included = included + 1
         else:
            sum_list.pop(0)
            sum_list.append(slic)
         new_slices[time] = add_slices(sum_list)
      self.slices = new_slices

   def time(self): return [time for time in sorted(self.slices)]

   def get_num_threads(self): return self.threads

   def print_first(self):
      s = min(self.slices, key=self.slices.get)
      print self.slices[s].data

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
      consumed_facts_file = os.path.join(path, "data.consumed_facts")
      if os.path.isfile(consumed_facts_file):
         consumed_facts = instrumentation_item(th)
         parse_numbers(consumed_facts, consumed_facts_file)
         bench.add_item("consumed_facts", th, consumed_facts)
      else:
         print "**WARNING** file", consumed_facts_file, "is missing"
      bytes_used_file = os.path.join(path, "data.bytes_used")
      if os.path.isfile(bytes_used_file):
         bytes_used = instrumentation_item(th)
         parse_numbers(bytes_used, bytes_used_file)
         bench.add_item("bytes_used", th, bytes_used)
      else:
         print "**WARNING** file", bytes_used_file, "is missing"
      all_transactions_file = os.path.join(path, "data.all_transactions")
      if os.path.isfile(all_transactions_file):
         all_transactions = instrumentation_item(th)
         parse_numbers(all_transactions, all_transactions_file)
         bench.add_item("all_transactions", th, all_transactions)
      else:
         print "**WARNING** file", all_transactions_file, "is missing"
      thread_transactions_file = os.path.join(path, "data.thread_transactions")
      if os.path.isfile(thread_transactions_file):
         thread_transactions = instrumentation_item(th)
         parse_numbers(thread_transactions, thread_transactions_file)
         bench.add_item("thread_transactions", th, thread_transactions)
      else:
         print "**WARNING** file", thread_transactions_file, "is missing"
      node_difference_file = os.path.join(path, "data.node_difference")
      if os.path.isfile(node_difference_file):
         node_difference = instrumentation_item(th)
         parse_numbers(node_difference, node_difference_file)
         bench.add_item("node_difference", th, node_difference)
      else:
         print "**WARNING** file", node_difference_file, "is missing"
      node_lock_ok_file = os.path.join(path, "data.node_lock_ok")
      node_lock_fail_file = os.path.join(path, "data.node_lock_fail")
      if os.path.isfile(node_lock_ok_file) and os.path.isfile(node_lock_fail_file):
         lock_ok = instrumentation_item(th)
         parse_numbers(lock_ok, node_lock_ok_file)
         lock_fail = instrumentation_item(th)
         parse_numbers(lock_fail, node_lock_fail_file)
         lock_ok.add(lock_fail)
         bench.add_item("node_locks", th, lock_ok)
      else:
         print "**WARNING** file", node_lock_ok_file, "missing"
      stolen_nodes_file = os.path.join(path, "data.stolen_nodes")
      if os.path.isfile(stolen_nodes_file):
         stolen_nodes = instrumentation_item(th)
         parse_numbers(stolen_nodes, stolen_nodes_file)
         bench.add_item("stolen_nodes", th, stolen_nodes)
      else:
         print "**WARNING** file", stolen_nodes_file, "missing"
      sent_facts_same_thread_file = os.path.join(path, "data.sent_facts_same_thread")
      sent_facts_other_thread_file = os.path.join(path, "data.sent_facts_other_thread")
      sent_facts_other_thread_now_file = os.path.join(path, "data.sent_facts_other_thread_now")
      if os.path.isfile(sent_facts_same_thread_file) and os.path.isfile(sent_facts_other_thread_file) and os.path.isfile(sent_facts_other_thread_now_file):
         sent_facts_same_thread = instrumentation_item(th)
         parse_numbers(sent_facts_same_thread, sent_facts_same_thread_file)
         sent_facts_other_thread = instrumentation_item(th)
         parse_numbers(sent_facts_other_thread, sent_facts_other_thread_file)
         sent_facts_other_thread_now = instrumentation_item(th)
         parse_numbers(sent_facts_other_thread_now, sent_facts_other_thread_now_file)
         sent_facts_other_thread.add(sent_facts_other_thread_now)
         bench.add_item("sent_facts_other_thread", th, sent_facts_other_thread)
         sent_facts_same_thread.add(sent_facts_other_thread)
         bench.add_item("sent_facts", th, sent_facts_same_thread)
      else:
         print "**WARNING** file", sent_facts_same_thread_file, "missing"
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
   ax.set_title(title.replace('_', ' ').title() + " / " + str(th) + " threads", fontsize=titlefontsize)
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
      if mean > 1:
         y = runningMeanFast(y0, mean)
      else:
         y = y0
      plot, = ax.plot(x, y,
         linestyle='-', color=colors[i % len(colors)])
      plots.append(plot)
   ax.legend(plots, [name for (name, _) in data], loc=1, fontsize=12, markerscale=2)
   file_name = sys.argv[2] + "/" + title + "/" + str(th) + ".png"
   try:
      os.makedirs(os.path.dirname(file_name))
   except OSError:
      pass
   print "Generating", file_name
   plt.savefig(file_name)
   plt.close(fig)

for th in [1, 2, 4, 6, 8, 10, 12, 14, 16, 20, 24]:
   state_data = []
   derived_facts_data = []
   consumed_facts_data = []
   pending_facts_data = []
   bytes_used_data = []
   node_locks_data = []
   stolen_nodes_data = []
   sent_facts_other_thread_data = []
   sent_facts_data = []
   thread_transactions_data = []
   all_transactions_data = []
   node_difference_data = []
   failed = False
   for bench in ins_set:
      try:
         state = bench.get_item("state", th)
         state_data.append((bench.get_name(), state))
         derived_facts = bench.get_item("derived_facts", th)
         derived_facts_data.append((bench.get_name(), derived_facts))
         consumed_facts = bench.get_item("consumed_facts", th)
         consumed_facts_data.append((bench.get_name(), consumed_facts))
         bytes_used = bench.get_item("bytes_used", th)
         bytes_used_data.append((bench.get_name(), bytes_used))
         node_locks = bench.get_item("node_locks", th)
         node_locks_data.append((bench.get_name(), node_locks))
         stolen_nodes = bench.get_item("stolen_nodes", th)
         stolen_nodes_data.append((bench.get_name(), stolen_nodes))
         sent_facts_other_thread = bench.get_item("sent_facts_other_thread", th)
         sent_facts_other_thread_data.append((bench.get_name(), sent_facts_other_thread))
         sent_facts = bench.get_item("sent_facts", th)
         sent_facts_data.append((bench.get_name(), sent_facts))
         thread_transactions = bench.get_item("thread_transactions", th)
         thread_transactions_data.append((bench.get_name(), thread_transactions))
         all_transactions = bench.get_item("all_transactions", th)
         all_transactions_data.append((bench.get_name(), all_transactions))
         node_difference = bench.get_item("node_difference", th)
         node_difference_data.append((bench.get_name(), node_difference))
      except KeyError:
         failed = True
   if not failed:
      plot_numeric_evolution("nodes", th, state_data) # average
      plot_numeric_evolution("derived_facts", th, derived_facts_data, True, 250) # average
      plot_numeric_evolution("consumed_facts", th, consumed_facts_data, True, 250) # average
      plot_numeric_evolution("bytes_used", th, bytes_used_data, True, 50) # average
      plot_numeric_evolution("node_locks", th, node_locks_data, True, 100)
      plot_numeric_evolution("stolen_nodes", th, stolen_nodes_data, True, 100)
      plot_numeric_evolution("sent_facts_other_thread", th, sent_facts_other_thread_data, True, 250)
      plot_numeric_evolution("sent_facts", th, sent_facts_data, True, 250)
      plot_numeric_evolution("thread_transactions", th, thread_transactions_data, True, 150)
      plot_numeric_evolution("all_transactions", th, all_transactions_data, True, 150)
      plot_numeric_evolution("node_difference", th, node_difference_data, True, 1)
