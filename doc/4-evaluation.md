# Experiment Setup

## Workload

评估数据均由YCSB[1]生成，共设计了15个trace。

| Benchmark  | 描述                     | 读写比   |
| ---------- | ------------------------ | -------- |
| Workload0  | 全部读                   | 1.0：0.0 |
| Workload1  | 一半读，一半插入         | 0.5：0.5 |
| Workload2  | 一半读，一半更新         | 0.5：0.5 |
| Workload3  | 大量读，少量插入         | 0.8：0.2 |
| Workload4  | 大量读，少量更新         | 0.8：0.2 |
| Workload5  | 大量读，少量插入         | 0.9：0.1 |
| Workload6  | 大量读，少量更新         | 0.9：0.1 |
| Workload7  | 少量读，大量插入         | 0.1：0.9 |
| Workload8  | 少量读，大量更新         | 0.1：0.9 |
| Workload9  | 少量读，大量插入         | 0.2：0.8 |
| Workload10 | 少量读，大量更新         | 0.2：0.8 |
| Workload11 | 全部插入                 | 0.0：1.0 |
| Workload12 | 全部更新                 | 0.0：1.0 |
| Workload13 | 全部扫描                 | 1.0：0.0 |
| Workload14 | 一半读，1/4插入，1/4更新 | 0.5：0.5 |

# Evaluation

## Latency

和LevelDB比，BiLSMTree在任何情况下均优于LevelDB，平均能够提高77%。主要原因是BiLSMTree也采用键值分离策略，降低Compaction时value的读出和重新写入所带来的额外代价。

和Wisckey比，BiLSMTree整体在读写均衡（Benchmark1、2、14）、读多写少（Benchmark3、4、5、6），或全读的情况（Benchmark0、13）下均优于Wisckey，平均能够提高70.14%，但在写多读少的情况下（Benchmark7、8、9、10）和全写的情况下（Benchmark11、12），BiLSMTree基本劣于Wisckey，平均降低了135.23%。其中写为更新写的情况下，（Benchmark7、9、11），平均降低了47.37%；写为插入写的情况下（Benchmark8、10、12），平均降低了223.10%。主要原因在读均多于或等于写、或全读的情况下，BiLSMTree通过将频繁访问的文件提升到高层，以降低未来对这些文件访问时的代价。在读少于或

## 读放大、写放大

## 内存中读的次数

## 从内存导出倒LSMTree的总数据量

## Compaction的总数据量

## 查找一个Key平均需要检查的文件个数

 

# Reference

1. Brian F. Cooper, Adam Silberstein, Erwin Tam, Raghu Ramakrishnan, and Russell Sears. 2010. Benchmarking cloud serving systems with YCSB. In Proceedings of the 1st ACM symposium on Cloud computing. ACM, 143–154. 