#/usr/bin/python
#
# Takes a benchmark step file and creates a CSV file
# that can be ploted using GNU plot.
# The resulting file plots the time needed to complete
# the computation as the data and number of workers increases.
#

import sys
import csv
import re
from lib_csv import *

if len(sys.argv) != 3:
   print "usage: csv_step.py <input bench file> <output plot file>"
   sys.exit(1)

in_file = str(sys.argv[1])
out_file = str(sys.argv[2])

def parse_bench_size(bench):
   vec = bench.split('_')
   size = vec[len(vec)-2]
   if len(size) == 2:
      return size[0]
   elif len(size) == 3:
      return size[:2]
   elif len(size) == 4:
      return size[:2]

def write_header(w):
   w.writerow(['#size', 'tl', 'tx', 'sin', 'mpi'])

def output_bench_data(size, serial, w, bench_data):
   tl = bench_data['tl'][size]
   tx = bench_data['tx'][size]
   sin = bench_data['sin'][size]
   mpi = bench_data['mpi'][size]
   w.writerow([size, tl, tx, sin, mpi])

def output_file(outfile):
   print outfile
   fp = open(outfile, 'wb')
   writer = csv.writer(fp, delimiter=' ')
   write_header(writer)
   serial = None
   for bench, bench_data in natural_dict_sort(data):
      size = parse_bench_size(bench)
      print size
      if serial is None:
         serial = bench_data['sl'][1]
      output_bench_data(int(size), serial, writer, bench_data)

read_csv_file(in_file)

output_file(out_file)
