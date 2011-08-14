#!/usr/bin/python
#
# This script takes a data.state file and creates a CSV file
# with the sum of the totals of all states for all threads
#
# EXAMPLE OUTPUT:
# state thread0 thread1 thread2 ...
# active x y z ..
# inactive w v l ..
# ...
#

import sys
import csv

if len(sys.argv) != 2:
   print "Usage: state_totals.py <data.state file>"
   sys.exit(1)
   
file = sys.argv[1]

f = open(file, 'rb')
active = {}
idle = {}
sched = {}
round = {}
comm = {}

def setup_data(num_threads):
   for i in range(0, num_threads-1):
      active[i] = 0
      idle[i] = 0
      sched[i] = 0
      round[i] = 0
      comm[i] = 0
def add_active(thread):
   cur = active[thread]
   active[thread] = cur + 1
def add_idle(thread):
   cur = idle[thread]
   idle[thread] = cur + 1
def add_sched(thread):
   cur = sched[thread]
   sched[thread] = cur + 1
def add_round(thread):
   cur = round[thread]
   round[thread] = cur + 1
def add_comm(thread):
   cur = comm[thread]
   comm[thread] = cur + 1
def get_info(name, data):
   ls = [name]
   for thread, total in data.iteritems():
      ls.append(str(total))
   return ls

first = True
num_threads = 0
for line in f:
   vec = line.split(' ')
   if first is True:
      num_threads = len(vec)-2
      setup_data(num_threads)
      first = False
      continue
   vec = vec[1:] # skip timestamp
   for i in range(0,len(vec)-1):
      state = vec[i]
      if state == '1':
         add_active(i)
      elif state == '0':
         add_idle(i)
      elif state == '2':
         add_sched(i)
      elif state == '3':
         add_round(i)
      elif state == '4':
         add_comm(i)
      else:
         print "FATAL ERROR"
         sys.exit(1)

f.close()

# write to new csv

outputfile = file + ".total"
writer = csv.writer(open(outputfile, 'wb'), delimiter=' ')

# write header
header = ['type']
for i in range(0, num_threads-1):
   header.append(str(i))
writer.writerow(header)

writer.writerow(get_info("active", active))
writer.writerow(get_info("idle", idle))
writer.writerow(get_info("sched", sched))
writer.writerow(get_info("round", round))
writer.writerow(get_info("comm", comm))