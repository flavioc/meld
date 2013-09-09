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

WRITE_COORD_IN_SPEEDUP = True

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

def make_coord(th, thp):
	return my_round(th / thp)
   
def write_header(writer):
	row = ['#cpu', 'th']
	if HAS_THS:
		row.append('ths')
	if HAS_THP:
		row.append('thp')
		if WRITE_COORD_IN_SPEEDUP:
			row.append('coord')
	if HAS_THX:
		row.append('thx')
		if WRITE_COORD_IN_SPEEDUP and HAS_THS:
			row.append('coords')
	writer.writerow(row)
   
def write_time_file(file, bench, bench_data):
   global CPUS
   writer = csv.writer(open(file, 'wb'), delimiter=' ')
   write_header(writer)
   for cpu in CPUS:
      th = bench_data['th'][cpu]
      writer.writerow([cpu, my_round(th)])
                           
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
		th = bench_data['th'][cpu]
		row = [cpu, make_speedup(th, serial)]
		if HAS_THS:
			ths = bench_data['ths'][cpu]
			row.append(make_speedup(ths, serial))
		if HAS_THP:
			thp = bench_data['thp'][cpu]
			row.append(make_speedup(thp, serial))
			if WRITE_COORD_IN_SPEEDUP:
				row.append(make_coord(th, thp))
		if HAS_THX:
			thx = bench_data['thx'][cpu]
			row.append(make_speedup(thx, serial))
			if WRITE_COORD_IN_SPEEDUP and HAS_THS:
				row.append(make_coord(thx, bench_data['ths'][cpu]))
		writer.writerow(row)

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
      th = bench_data['th'][cpu]
      writer.writerow([cpu, make_efficiency(th, serial, cpu)])

def write_coord_file(file, bench, bench_data):
	global CPUS
	writer = csv.writer(open(file, 'wb'), delimiter=' ')
	writer.writerow(['#cpu', 'speedup'])
	for cpu in CPUS:
		th = bench_data['th'][cpu]
		thp = bench_data['thp'][cpu]
		writer.writerow([cpu, make_coord(th, thp)])

def write_speedup_files():
   for bench, bench_data in data.iteritems():
      write_speedup_file(dir + "/speedup." + bench, bench, bench_data)
      
def write_time_files():
   for bench, bench_data in data.iteritems():
      write_time_file(dir + "/time." + bench, bench, bench_data)

def write_efficiency_files():
   for bench, bench_data in data.iteritems():
      write_efficiency_file(dir + "/efficiency." + bench, bench, bench_data)

def write_coord_files():
	for bench, bench_data in data.iteritems():
		write_coord_file(dir + "/coord." + bench, bench, bench_data)

def detect(typ):
	for bench, bench_data in data.iteritems():
		try:
			c = bench_data[typ]
		except KeyError:
			return False
	return True
		
      
if len(sys.argv) < 3:
   print "Usage: genenerate_speedup_data.py <output dir> <bench file>"
   sys.exit(1)

dir = str(sys.argv[1])
file = str(sys.argv[2])

read_csv_file(file)
HAS_THS = detect('ths')
HAS_THP = detect('thp')
HAS_THX = detect('thx')

mkdir_p(dir)
write_speedup_files()
write_time_files()
if HAS_THP:
	write_coord_files()
#write_efficiency_files()
