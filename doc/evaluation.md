# Experiment Setup

## Workload

评估数据均由YCSB[1]生成，共设计了15个trace。

| 名字      | 描述                     | 读写比   |
| --------- | ------------------------ | -------- |
| WorkloadA | 全部读                   | 1.0：0.0 |
| WorkloadB | 一半读，一半插入         | 0.5：0.5 |
| WorkloadC | 一半读，一半更新         | 0.5：0.5 |
| WorkloadD | 大量读，少量插入         | 0.8：0.2 |
| WorkloadE | 大量读，少量更新         | 0.8：0.2 |
| WorkloadF | 大量读，少量插入         | 0.9：0.1 |
| WorkloadG | 大量读，少量更新         | 0.9：0.1 |
| WorkloadH | 少量读，大量插入         | 0.1：0.9 |
| WorkloadI | 少量读，大量更新         | 0.1：0.9 |
| WorkloadJ | 少量读，大量插入         | 0.2：0.8 |
| WorkloadK | 少量读，大量更新         | 0.2：0.8 |
| WorkloadL | 全部插入                 | 0.0：1.0 |
| WorkloadM | 全部更新                 | 0.0：1.0 |
| WorkloadN | 全部扫描                 | 1.0：0.0 |
| WorkloadO | 一半读，1/4插入，1/4更新 | 0.5：0.5 |





# Reference

1. Brian F. Cooper, Adam Silberstein, Erwin Tam, Raghu Ramakrishnan, and Russell Sears. 2010. Benchmarking cloud serving systems with YCSB. In Proceedings of the 1st ACM symposium on Cloud computing. ACM, 143–154. 