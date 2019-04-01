import os
import random as rd
from functools import reduce

distribution_attr = ['operationcount', 'readproportion', 'updateproportion', 'insertproportion', 'scanproportion']
distribution = [(300000, 1.00, 0.00, 0.00, 0.00),  # A 0  100% read
                (300000, 0.50, 0.50, 0.00, 0.00),  # B 1  50%  read 50% update
                (300000, 0.50, 0.00, 0.50, 0.00),  # C 2  50%  read 50% insert
                (300000, 0.80, 0.20, 0.00, 0.00),  # D 3  80%  read 20% update
                (300000, 0.80, 0.00, 0.20, 0.00),  # E 4  80%  read 20% insert
                (300000, 0.90, 0.10, 0.00, 0.00),  # F 6  90%  read 10% update
                (300000, 0.90, 0.00, 0.10, 0.00),  # G 5  90%  read 10% insert
                (300000, 0.10, 0.90, 0.00, 0.00),  # H 7  10%  read 90% update
                (300000, 0.10, 0.00, 0.90, 0.00),  # I 8  10%  read 90% insert
                (300000, 0.20, 0.80, 0.00, 0.00),  # J 9  20%  read 80% update
                (300000, 0.20, 0.00, 0.80, 0.00),  # K 10 20%  read 80% insert
                (300000, 0.00, 1.00, 0.00, 0.00),  # L 11 100% update
                (300000, 0.00, 0.00, 1.00, 0.00),  # M 12 100% insert
                (3000, 0.00, 0.00, 0.00, 1.00),    # N 13 100% scan
                (300000, 0.50, 0.25, 0.25, 0.00)]  # O 14 50%  read 25% update 25% insert

attributes = {
  'recordcount': 50000,
  'workload': 'com.yahoo.ycsb.workloads.CoreWorkload',
  'readallfields': 'true',
  'requestdistribution': 'zipfian' # latest, uniform
}
workload_num = len(distribution)
key_v_value = 64
padding = '&'

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
  n = len(arr)
  for i in range(n):
    j = rd.randint(i, n - 1)
    v = arr[i]
    arr[i] = arr[j]
    arr[j] = v

def read(in_filename, out_filename):
  print('Process ' + in_filename)
  replace_key = [':', ',', ';', '$', ' ', '\t']
  f = open(in_filename, 'r')
  lines = f.readlines()
  f.close()
  result = []
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
      if len(value) < len(key) * key_v_value:
        for u in range(len(key) * key_v_value - len(value)):
          value = value + padding
      else:
        value = value[:len(key) * key_v_value]
      if op == 'SCAN':
        suffix = ls[3]
        value = key[:len(key) - len(suffix)]
        for s in suffix:
          value = value + s
        if int(key) > int(value):
          t = key
          key = value
          value = t
      result.append([op, key, value])
  shuffle(result)
  f = open(out_filename, 'a')
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
