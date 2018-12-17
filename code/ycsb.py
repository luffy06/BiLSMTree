import os
import numpy as np
from functools import reduce

distribution_attr = ['operationcount', 'readproportion', 'updateproportion', 'insertproportion', 'scanproportion', 'readmodifywriteproportion']
distribution = [(100000, 0.50, 0.50, 0.00, 0.00, 0.00), # 0  a 50% read 50% update
                (60000, 0.95, 0.05, 0.00, 0.00, 0.00),  # 1  b 95% read 5% update
                (60000, 0.95, 0.05, 0.00, 0.00, 0.00),  # 2    5% read 95% update
                (60000, 1.00, 0.00, 0.00, 0.00, 0.00),  # 3  c 100% read
                (60000, 0.50, 0.50, 0.00, 0.00, 0.00),  # 4    50% read 50% insert
                (60000, 0.95, 0.00, 0.00, 0.05, 0.00),  # 5  d 95% read 5% insert
                (60000, 0.95, 0.00, 0.00, 0.05, 0.00),  # 6  d 5% read 95% insert
                (60000, 0.00, 0.99, 0.00, 0.01, 0.00),  # 7    50% scan 50% update
                (3000, 0.00, 0.05, 0.00, 0.95, 0.00),   # 8    95% scan 5% update
                (3000, 0.00, 0.05, 0.00, 0.95, 0.00),   # 9    5% scan 95% update
                (6000, 0.00, 0.00, 0.00, 1.00, 0.00),   # 10   100% scan
                (60000, 0.00, 0.00, 0.00, 0.01, 0.99),  # 11   50% scan 50% insert
                (3000, 0.00, 0.00, 0.05, 0.95, 0.00),   # 12 e 95% scan 5% insert
                (3000, 0.00, 0.00, 0.05, 0.95, 0.00),   # 13 e 5% scan 95% insert
                (60000, 0.99, 0.00, 0.00, 0.01, 0.00)]  # 14   50% scan 50% read

attributes = {
  'recordcount': 50000,
  'workload': 'com.yahoo.ycsb.workloads.CoreWorkload',
  'readallfields': 'true',
  'requestdistribution': 'zipfian' # latest, uniform
}
workload_num = len(distribution)
value_len_max = 1000

def generate_workload(project_path):
  for i in range(workload_num):
    dist = distribution[i]
    filename = project_path + 'workloads/workload' + str(i)
    f = open(filename, 'w')
    for k, v in attributes.items():
      f.write(k + '=' + str(v) + '\n')
    for j in range(len(distribution_attr)):
      f.write(distribution_attr[j] + '=' + str(dist[j]) + '\n')
    f.close()

def get_data(project_path):
  cmdlist = []

  for i in range(workload_num):
    cmd = project_path + 'bin/ycsb.sh load basic -P ' + project_path + 'workloads/workload' + str(i) + ' > ' + dirname + 'data' + str(i) + '_load.in'
    cmdlist.append(cmd)
    cmd = project_path + 'bin/ycsb.sh run basic -P ' + project_path + 'workloads/workload' + str(i) + ' > ' + dirname + 'data' + str(i) + '_run.in'
    cmdlist.append(cmd)

  for cmd in cmdlist:
    print('CMD:' + cmd)
    os.system(cmd)

def clean_data(project_path, dirname, prefix, suffix):
  cmdlist = []
  for i in range(workload_num):
    cmd1 = 'rm ' + dirname + prefix + str(i) + '_load' + suffix
    cmd2 = 'rm ' + dirname + prefix + str(i) + '_run' + suffix
    cmd3 = 'rm ' + project_path + 'workloads/workload' + str(i)
    cmdlist.append(cmd1)
    cmdlist.append(cmd2)
    cmdlist.append(cmd3)

  for cmd in cmdlist:
    print('CMD:' + cmd)
    os.system(cmd)

def shuffle(arr):
  np.random.shuffle(arr)

def read(in_filename, out_filename):
  replace_key = [':', ',', ';', '$', ' ', '\t']
  f = open(in_filename, 'r')
  lines = f.readlines()
  f.close()
  load = []
  result = []
  j = 0
  for i, l in enumerate(lines):
    if l.startswith('INSERT') or l.startswith('UPDATE') or l.startswith('SCAN') or l.startswith('READ'):
      ls = l.split()
      op = ls[0]
      key = ls[2][4:]
      if op == 'SCAN':
        st = 4
      else:
        st = 3
      value = ''
      for j in range(st, len(ls)):
        value = value + ls[j] + ' '
      value = value.strip()
      for k in replace_key:
        value = value.replace(k, '0')
      # value = value[:value_len_max]
      if op == 'SCAN':
        suffix = ls[3]
        value = key[:len(key) - len(suffix)]
        for s in suffix:
          value = value + s
        if int(key) > int(value):
          t = key
          key = value
          value = t
      if j < attributes['recordcount']:
        load.append([op, key, value])
      else:
        result.append([op, key, value])
  shuffle(load)
  shuffle(result)
  f = open(out_filename, 'a')
  for i, l in enumerate(load):
    line = reduce(lambda x, w: x + '\t' + w, l, '').strip()
    f.write(line + '\n')
  for i, l in enumerate(result):
    line = reduce(lambda x, w: x + '\t' + w, l, '').strip()
    f.write(line + '\n')
  f.close()

if __name__ == '__main__':
  project_path = 'lib/YCSB/'
  dirname = 'data/'
  prefix = 'data'
  suffix = '.in'
  print('GENERATE WORKLOAD')
  generate_workload(project_path)
  print('GET DATA')
  get_data(project_path)
  print('READ DATA')
  for i in range(workload_num):
    in_filename_load = dirname + prefix + str(i) + '_load' + suffix
    in_filename_run = dirname + prefix + str(i) + '_run' + suffix
    out_filename = dirname + prefix + str(i) + suffix
    if os.path.exists(out_filename):
      os.remove(out_filename)
    read(in_filename_load, out_filename)
    read(in_filename_run, out_filename)
  print('CLEAN DATA')
  clean_data(project_path, dirname, prefix, suffix)
