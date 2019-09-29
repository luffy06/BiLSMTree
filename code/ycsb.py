import os
import random as rd
from functools import reduce

distribution_attr = ['operationcount', 'readproportion', 'updateproportion', 'insertproportion', 'scanproportion']
distribution = [
                # (5000000, 1.00, 0.00, 0.00, 0.00),  # A 1  100% read
                # (3000, 0.00, 0.00, 0.00, 1.00),    # N    100% scan
                (1000000, 0.90, 0.10, 0.00, 0.00),  # F 2  90%  read 10% update
                (1000000, 0.90, 0.00, 0.10, 0.00),  # G 3  90%  read 10% insert
                (1000000, 0.80, 0.20, 0.00, 0.00),  # D 4  80%  read 20% update
                (1000000, 0.80, 0.00, 0.20, 0.00),  # E 5  80%  read 20% insert
                (1000000, 0.50, 0.50, 0.00, 0.00),  # B 6  50%  read 50% update
                (1000000, 0.50, 0.00, 0.50, 0.00),  # C 7  50%  read 50% insert
                # (5000000, 0.50, 0.25, 0.25, 0.00),  # O 8  50%  read 25% update 25% insert
                (1000000, 0.20, 0.80, 0.00, 0.00),  # J 9  20%  read 80% update
                (1000000, 0.20, 0.00, 0.80, 0.00),  # K 10 20%  read 80% insert
                # (800000, 0.10, 0.90, 0.00, 0.00),  # H 7  10%  read 90% update
                # (800000, 0.10, 0.00, 0.90, 0.00),  # I 8  10%  read 90% insert
                # (5000000, 0.00, 1.00, 0.00, 0.00),  # L 11 100% update
                # (5000000, 0.00, 0.00, 1.00, 0.00)   # M 12 100% insert
                ]
attributes = {
  'recordcount': 100000,
  'workload': 'com.yahoo.ycsb.workloads.CoreWorkload',
  'readallfields': 'true',
  'requestdistribution': 'zipfian' # latest, uniform
}
workload_num = len(distribution)
key_size = 16       # 64B
key_v_value = 64    # 1KB
padding = '&'
limited = 500000

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

def write_result(result, filename):
  f = open(filename, 'a')
  for i, l in enumerate(result):
    line = reduce(lambda x, w: x + '\t' + w, l, '').strip()
    f.write(line + '\n')
  f.close()

def read(in_filename, out_filename):
  print('Process ' + in_filename)
  replace_key = [':', ',', ';', '$', ' ', '\t']
  f = open(in_filename, 'r')
  result = []
  i = 0
  for l in f:
    if l.startswith('INSERT') or l.startswith('UPDATE') or l.startswith('SCAN') or l.startswith('READ'):
      ls = l.strip().split()
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
        add_value = ""
        for u in range(len(key) * key_v_value - len(value)):
          add_value = add_value + padding
        value = value + add_value
      else:
        value = value[:len(key) * key_v_value]
      if op == 'SCAN':
        suffix = ls[3]
        value = key[:len(key) - len(suffix)] + suffix
        if int(key) > int(value):
          t = key
          key = value
          value = t
      if len(key) < key_size:
        for i in range(key_size - len(key)):
          key = '0' + key
      if len(key) > key_size:
        key = key[len(key) - key_size:len(key)]
      result.append([op, key, value])
      i = i + 1
      if len(result) >= limited:
        write_result(result, out_filename)
        result = []
  write_result(result, out_filename)

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
