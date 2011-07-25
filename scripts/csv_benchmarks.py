#!/usr/bin/python
#
# This takes as input a benchmark output and
# creates several CSV files per benchmark with all the
# scheduling strategies.
#
# It currently outputs the following statistics:
# Time
# Speedup
# Efficiency

import sys
import csv
import os
import errno

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST:
            pass
        else: raise

def get_sched_name(sched):
   if sched == 'sl':
      return 'sl'
   if sched[:2] == 'tl':
      return 'tl'
   if sched[:2] == 'td':
      return 'td'
   if sched[:2] == 'tx':
      return 'tx'
   if sched[:9] == 'mpistatic':
      return 'mpi'
   return 'sin'
      
def get_sched_threads(sched):
   if sched == 'sl': return 1
   part = sched[:2]
   if part == 'tl' or part == 'td' or part == 'tx':
      return int(sched[2:])
   elif sched[:9] == 'mpistatic':
      rest = sched[9:]
      vec = rest.split('/')
      return int(vec[0])
   else:
      return int(sched[3:])
      
def build_results(vec):
   new_vec = vec[2:]
   sum = 0
   i = 0
   total = 0
   while True:
      n = new_vec[i]
      i = i + 1
      if n == '':
         continue
      if n == '->':
         return float(sum / total)
      sum = sum + int(n)
      total = total + 1
      
data = {}

def add_result(name, sched, threads, result):
   global data
   if result == 0:
      result = 1
   try:
      bench_data = data[name]
      try:
         sched_data = bench_data[sched]
         sched_data[threads] = result
      except KeyError:
         sched_data = {threads: result}
         bench_data[sched] = sched_data
   except KeyError:
      sched_data = {threads: result}
      bench_data = {sched: sched_data}
      data[name] = bench_data
      
def print_results():
   global data
   for bench, bench_data in data.iteritems():
        for sched, sched_data in bench_data.iteritems():
           for threads, result in sched_data.iteritems():
              print bench, sched, threads, result

def lookup_serial_result(name):
   global data
   return data[name]['sl'][1]
      
def my_round(time):
   return round(time, 3)
   
def make_speedup(time, serial):
   return my_round(serial / time)
   
def make_efficiency(time, serial, cpu):
   return my_round((serial / time) / float(cpu))
   
CPUS = [1, 2, 4, 6, 8, 10, 12, 14, 16]

def write_header(writer):
   writer.writerow(['#cpu', 'tl', 'td', 'tx', 'sin', 'mpi'])
   
def write_time_file(file, bench, bench_data):
   global CPUS
   writer = csv.writer(open(file, 'wb'), delimiter=' ')
   write_header(writer)
   for cpu in CPUS:
      tl = bench_data['tl'][cpu]
      td = bench_data['td'][cpu]
      tx = bench_data['tx'][cpu]
      sin = bench_data['sin'][cpu]
      mpi = bench_data['mpi'][cpu]
      writer.writerow([cpu, my_round(tl),
                            my_round(td),
                            my_round(tx),
                            my_round(sin),
                            my_round(mpi)])
                           
def write_speedup_file(file, bench, bench_data):
   global CPUS
   serial = 0
   try:
      serial = lookup_serial_result(bench)
   except KeyError:
      print "Fail to get serial time for " + bench
      sys.exit(1)
   writer = csv.writer(open(file, 'wb'), delimiter=' ')
   write_header(writer)
   for cpu in CPUS:
      tl = bench_data['tl'][cpu]
      td = bench_data['td'][cpu]
      tx = bench_data['tx'][cpu]
      sin = bench_data['sin'][cpu]
      mpi = bench_data['mpi'][cpu]
      writer.writerow([cpu, make_speedup(tl, serial),
                            make_speedup(td, serial),
                            make_speedup(tx, serial),
                            make_speedup(sin, serial),
                            make_speedup(mpi, serial)])

def write_efficiency_file(file, bench, bench_data):
   global CPUS
   serial = 0
   try:
      serial = lookup_serial_result(bench)
   except KeyError:
      print "Fail to get serial time for " + bench
      sys.exit(1)
   writer = csv.writer(open(file, 'wb'), delimiter=' ')
   write_header(writer)
   for cpu in CPUS:
      tl = bench_data['tl'][cpu]
      td = bench_data['td'][cpu]
      tx = bench_data['tx'][cpu]
      sin = bench_data['sin'][cpu]
      mpi = bench_data['mpi'][cpu]
      writer.writerow([cpu, make_efficiency(tl, serial, cpu),
                            make_efficiency(td, serial, cpu),
                            make_efficiency(tx, serial, cpu),
                            make_efficiency(sin, serial, cpu),
                            make_efficiency(mpi, serial, cpu)])
                            
def write_speedup_files():
   for bench, bench_data in data.iteritems():
      write_speedup_file(dir + "/speedup." + bench, bench, bench_data)
      
def write_time_files():
   for bench, bench_data in data.iteritems():
      write_time_file(dir + "/time." + bench, bench, bench_data)

def write_efficiency_files():
   for bench, bench_data in data.iteritems():
      write_efficiency_file(dir + "/efficiency." + bench, bench, bench_data)
      
if len(sys.argv) < 3:
   print "Usage: genenerate_speedup_data.py <output dir> <bench file>"
   sys.exit(1)

dir = str(sys.argv[1])
file = str(sys.argv[2])

f = open(file, 'rb')

for line in f:
   line = line.rstrip('\n')
   vec = line.split(' ')
   name = vec[0]
   sched = vec[1]
   sched_name = get_sched_name(sched)
   sched_threads = get_sched_threads(sched)
   result = build_results(vec)
   add_result(name, sched_name, sched_threads, result)

mkdir_p(dir)
write_speedup_files()
write_time_files()
write_efficiency_files()