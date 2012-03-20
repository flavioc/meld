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
from lib_csv import *

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
   
def write_header(writer):
   writer.writerow(['#cpu', 'tl', 'tx', 'sin', 'mpi'])
   
def write_time_file(file, bench, bench_data):
   global CPUS
   writer = csv.writer(open(file, 'wb'), delimiter=' ')
   write_header(writer)
   for cpu in CPUS:
      tl = bench_data['tl'][cpu]
      tx = bench_data['tx'][cpu]
      sin = bench_data['sin'][cpu]
      mpi = bench_data['mpi'][cpu]
      writer.writerow([cpu, my_round(tl),
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
      tx = bench_data['tx'][cpu]
      sin = bench_data['sin'][cpu]
      mpi = bench_data['mpi'][cpu]
      writer.writerow([cpu, make_speedup(tl, serial),
                            make_speedup(tx, serial),
                            make_speedup(sin, serial),
                            make_speedup(mpi, serial)])

def write_efficiency_file(file, bench, bench_data):
   global CPUS
   serial = 0
   try:
      serial = lookup_serial_result(bench)
   except KeyError:
      print "Failed to get serial time for " + bench
      sys.exit(1)
   writer = csv.writer(open(file, 'wb'), delimiter=' ')
   write_header(writer)
   for cpu in CPUS:
      tl = bench_data['tl'][cpu]
      tx = bench_data['tx'][cpu]
      sin = bench_data['sin'][cpu]
      mpi = bench_data['mpi'][cpu]
      writer.writerow([cpu, make_efficiency(tl, serial, cpu),
                            make_efficiency(tx, serial, cpu),
                            make_efficiency(sin, serial, cpu),
                            make_efficiency(mpi, serial, cpu)])

def write_mix_speedup_file(file, bench, bench_data):
   global CPUS
   serial = 0
   try:
      serial = lookup_serial_result(bench)
   except KeyError:
      print "Failed to get serial time for " + bench
      sys.exit(1)
   writer = csv.writer(open(file + "_curves", 'wb'), delimiter=' ')
   writer.writerow(['#cpu', 'mpi', 'sin'])
   for cpu in CPUS:
      mpi = bench_data['mpi'][cpu]
      sin = bench_data['sin'][cpu]
      writer.writerow([cpu, make_speedup(mpi, serial), make_speedup(sin, serial)])
   writer = csv.writer(open(file + "_points", 'wb'), delimiter=' ')
   writer.writerow(['#total', 'speedup', 'type'])
   for cpu, time in bench_data['mix'].iteritems():
      vec = cpu.split('/')
      proc = int(vec[0])
      threads = int(vec[1])
      writer.writerow([proc * threads, make_speedup(time, serial), str(proc) + "x" + str(threads)])
   
def write_speedup_files():
   for bench, bench_data in data.iteritems():
      write_speedup_file(dir + "/speedup." + bench, bench, bench_data)
      
def write_time_files():
   for bench, bench_data in data.iteritems():
      write_time_file(dir + "/time." + bench, bench, bench_data)

def write_efficiency_files():
   for bench, bench_data in data.iteritems():
      write_efficiency_file(dir + "/efficiency." + bench, bench, bench_data)
      
def write_mix_speedup_files():
   for bench, bench_data in data.iteritems():
      try:
         mix = bench_data['mix']
         write_mix_speedup_file(dir + "/mix." + bench, bench, bench_data)
      except KeyError:
         continue
      
if len(sys.argv) < 3:
   print "Usage: genenerate_speedup_data.py <output dir> <bench file>"
   sys.exit(1)

dir = str(sys.argv[1])
file = str(sys.argv[2])

read_csv_file(file)

mkdir_p(dir)
write_speedup_files()
write_time_files()
write_efficiency_files()
write_mix_speedup_files()
