import os, sys

def main():
  if len(sys.argv) != 5:
    print('Python Parameter Error')
    sys.exit()
  
  algo = sys.argv[1]
  data = sys.argv[2]
  source_name = sys.argv[3]
  trace_path = sys.argv[4]
  trace_name = os.path.join(trace_path, algo + '_' + data)
  
  print(trace_name)
  print(source_name)
  f = open(source_name, 'r')
  lines = f.readlines()
  f.close()

  f = open(trace_name, 'w')
  f.write(lines[-1])
  for i in range(len(lines) - 1):
    f.write(lines[i])
  f.close()


if __name__ == '__main__':
  main()