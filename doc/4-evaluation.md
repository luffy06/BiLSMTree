# Experiment Setup

## Workloads

评估数据均由YCSB[1]生成，共设计了15个trace。YCSB是一个框架，它的目的是为了便利比较不同的新生代的云数据服务系统的性能。（with the goal of facilitating performance comparisons of the new generation of cloud data serving systems.）YCSB测试时分为两个阶段，Load和Running阶段。Load阶段往待测试的数据库中加载数据，Running阶段在Load阶段已加载的数据的基础上进行不同操作的测试。操作的类别包括随机读（read），范围读（range lookup），插入写（insert），更新写（update）。

| workload   | 描述                     | 读写比   |
| ---------- | ------------------------ | -------- |
| Workload0  | 全部读                   | 1.0：0.0 |
| Workload1  | 一半读，一半插入         | 0.5：0.5 |
| Workload2  | 一半读，一半更新         | 0.5：0.5 |
| Workload3  | 大量读，少量更新         | 0.8：0.2 |
| Workload4  | 大量读，少量插入         | 0.8：0.2 |
| Workload5  | 大量读，少量更新         | 0.9：0.1 |
| Workload6  | 大量读，少量插入         | 0.9：0.1 |
| Workload7  | 少量读，大量更新         | 0.1：0.9 |
| Wokrload8  | 少量读，大量插入         | 0.1：0.9 |
| Workload9  | 少量读，大量更新         | 0.2：0.8 |
| Workload10 | 少量读，大量插入         | 0.2：0.8 |
| Workload11 | 全部更新                 | 0.0：1.0 |
| Workload12 | 全部插入                 | 0.0：1.0 |
| Workload13 | 全部扫描                 | 1.0：0.0 |
| Workload14 | 一半读，1/4插入，1/4更新 | 0.5：0.5 |

## 对比

我们将FSLSM-Tree分别和具有代表性的方法进行了比较，一个是Wisckey，另一个是LevelDB。Wisckey首个提出键值分离思想，显著的提升了写性能。LevelDB是当前最流行的基于LSM-Tree结构的数据库。

## 性能指标

我们比较了以下7个性能指标：

* 总延迟
* 读放大
* 写放大
* Compaction的总数据量
* 查找一个key平均需要检查的文件个数

这5个性能指标能够全面的反映FSLSM-Tree的读写性能。

# Experiment Result

## Latency

和LevelDB比，FSLSM-Tree在任何情况下均优于LevelDB，平均能够提高72.13%。主要原因是对于写多的workload，FSLSM-Tree也采用键值分离策略，降低Compaction时value的读出和重新写入所带来的额外代价；对于读多的workload，数据的上浮也为FSLSM-Tree带来了足够多的提升。

和Wisckey比，

* 在读写均衡（workload1、2、12）、读多写少（workload3、4、5、6），或全读的情况（workload0、11）下FSLSM-Tree均优于Wisckey，平均能够提高55.47%，主要原因在于上浮机制减少了额外的读；
* 在写多读少的情况下（workload7、8），FSLSM-Tree劣于Wisckey，降低了20.46%，主要原因是伸展机制与上浮频率的不匹配，伸展机制识别出当前workload中有10%的读，并据此调整了LSM-Tree的形状，但由于读频率较少，上浮次数较少，减少的额外读不足以抵消伸展后产生的overhead。
* 在全写的情况下（workload10、11），FSLSM-Tree与Wisckey算法相同，因为此时FSLSM-Tree识别出为全写的特性，所以不进行上浮和伸展操作。

## 读放大、写放大

FSLSM-Tree有效的降低了读放大，同时也没有产生过多额外写放大，相比Wisckey平均降低了4.81倍的读放大，相比LevelDB平均降低了9.66倍的读放大。因为Wisckey显著的降低了LevelDB的写放大，所以FSLSM-Tree在Wisckey的基础上，基本保持了相同的写放大，最高增加了2.04倍写放大，但又保持了对LevelDB的显著提升。读放大的显著提升主要得益于上浮后，减少了查找的文件个数，从而减少了多余的读数据量。

workload7和8产生了更多的读放大，但减少了写放大。主要的原因是伸展机制使得高层驻留了更多的文件，减少了Compaction的数据量，但增大了文件额外读的数据量。

## Compaction的总数据量

虽然FSLSM-Tree通过伸展机制尽可能的减少因上浮所引起的Compaction，但还是会产生一些额外的Compaction操作，最多的workload相比于Wisckey将会产生多达4倍的Compaction数据量。但从平均来看，FSLSM-Tree仅仅产生了1.2倍的Compaction数据量。FSLM-Tree因为和Wisckey一样，没有存储value，所以Compaction的数据量能够得到大大的减少。

## 查找一个Key平均需要检查的文件个数

FSLSM-Tree显著的减少了查找一个Key平均需要检查的文件个数，平均只需要检查3.40个文件就能够查到真正的value，而Wisckey和LevelDB平均需要检查4.36和4.40个文件才能够查到真正的value。因为FSLSM-Tree将文件上浮到高层，同时加上伸展机制对文件的延缓Compaction作用，使得查找文件的个数的到显著的减少。 

# Reference

1. Brian F. Cooper, Adam Silberstein, Erwin Tam, Raghu Ramakrishnan, and Russell Sears. 2010. workloading cloud serving systems with YCSB. In Proceedings of the 1st ACM symposium on Cloud computing. ACM, 143–154. 