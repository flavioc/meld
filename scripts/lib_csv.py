
import re
import os
import errno

data = {}
CPUS = [1, 2, 4, 6, 8, 10, 12, 14, 16]

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
   if sched[:9] == 'mpisingle':
      return 'mix'
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
   elif sched[:9] == 'mpisingle':
      return sched[9:]
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
      
def read_csv_file(file):
   f = open(file, 'rb')

   for line in f:
      line = line.rstrip('\n')
      vec = line.split(' ')
      name = vec[0]
      sched = vec[1]
      sched_name = get_sched_name(sched)
      sched_threads = get_sched_threads(sched)
      result = build_results(vec)
      print sched_name, sched_threads
      add_result(name, sched_name, sched_threads, result)

def natural_dict_sort(dic):
   convert = lambda text: int(text) if text.isdigit() else text 
   alphanum_key = lambda key: [ convert(c) for c in re.split('([0-9]+)', key) ]
   return sorted(data.iteritems(), key=lambda (k,v): alphanum_key(k))

def lookup_serial_result(name):
   global data
   return data[name]['sl'][1]
