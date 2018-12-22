import xlwt, xlsxwriter
import os

resultdir = 'result'
resultexcel = 'result.xlsx'
algorithms = ['BiLSMTree', 'Wisckey', 'LevelDB']
attributes = ['READ', 'UPDATE', 'INSERT', 'SCAN', 'SCAN_READ']
metrics = ['LATENCY', 'READ_TIMES', 'WRITE_TIMES', 'ERASE_TIMES', 'READ_FILES', 
            'READ_SIZE', 'AVG_READ_SIZE', 'WRITE_FILES', 'WRITE_SIZE', 
            'AVG_WRITE_SIZE', 'MINOR_COMPACTION', 'MINOR_COMPACTION_SIZE', 
            'AVG_MINOR_COMPACTION_SIZE', 'MAJOR_COMPACTION', 'MAJOR_COMPACTION_SIZE', 
            'AVG_MAJOR_COMPACTION_SIZE', 'AVERAGE_CHECK_TIMES'];

def parse_filename(filename):
  names = filename.split('.')
  assert(len(names) == 2)
  return (names[0], names[1])

def parse_trace(line):
  trace_name = line.strip().split(' ')[1]
  trace_name = trace_name.split('.')[0][4:]
  return int(trace_name)

def parse_content(line):
  lines = line.strip().split(' ')
  data = {}
  for l in lines:
    d = l.split(':')
    assert(len(d) == 2)
    data[d[0]] = d[1]
  return data

def parse_filecontent(filename, algo, attribute, resultmap):
  f = open(filename, 'r')
  lines = f.readlines();
  f.close()
  assert(len(lines) % 2 == 0)
  attr = []
  for i in range(0, len(lines), 2):
    trace_id = parse_trace(lines[i])
    data = parse_content(lines[i + 1])
    attr.append([trace_id, data['READ'], data['UPDATE'], data['INSERT'], data['SCAN'], data['SCAN_READ']])
    for m in metrics:
      resultmap[m][algo].append([trace_id, data[m]])
  attr.sort(key=lambda x: x[0])
  if len(attribute) == 0:
    for r in attr:
      row = []
      for v in r:
        row.append(v)
      attribute.append(row)
  else:
    assert(len(attribute) == len(attr))
    for i in range(len(attr)):
      assert(len(attribute[i]) == len(attr[i]))
      for j in range(len(attr[i])):
        assert(attribute[i][j] == attr[i][j])

def load_result(attr, resultmap):
  wb = xlsxwriter.Workbook(os.path.join(resultdir, resultexcel))
  st = wb.add_worksheet()
  r = 0
  for m in metrics:
    # write metrics name
    st.write(r, 0, m)
    r = r + 1

    # write attributes
    st.write(r, 0, 'DATA')
    for j, a in enumerate(attributes):
      st.write(r, j + 1, a)
    for j, a in enumerate(algorithms):
      st.write(r, j + 1 + len(attributes), a)
    r = r + 1

    # write trace
    for i, a in enumerate(attr):
      for j, v in enumerate(a):
        st.write(r, j, v)
      for j, algo in enumerate(algorithms):
        assert(len(resultmap[m][algo]) == len(attr))
        st.write(r, j + len(a), resultmap[m][algo][i][1])
      r = r + 1

    r = r + 1
  wb.close()

def main():
  resultmap = {}
  attr = []

  for m in metrics:
    resultmap[m] = {}
    for algo in algorithms:
      resultmap[m][algo] = []

  for f in os.listdir(resultdir):
    algo, suffix = parse_filename(f)
    if suffix != 'out':
      continue
    if algo in algorithms:
      print('Loading ' + algo)
      parse_filecontent(os.path.join(resultdir, f), algo, attr, resultmap)

  for m in metrics:
    for algo in algorithms:
      resultmap[m][algo].sort(key=lambda x: x[0])
  
  load_result(attr, resultmap)

if __name__ == '__main__':
  main()