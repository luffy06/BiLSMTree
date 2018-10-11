# Ideas

## Motivation

目标：**基于Flash的LevelDB，在不影响原有写性能的基础上优化读放大问题**。

### Read Amplification in LSM-tree

用LevelDB来解释LSM-tree中的读放大问题，在LevelDB查找一个Key的步骤可以分为在内存查找和在外存查找两步：

1. 在内存中，首先查找MemTable，若没有找到；检查Immutable MemTable是否存在，若存在，在Immutable MemTable中查找。

2. 若在内存中均未找到，则在外存中查找。根据内存中所存储的所有SSTable的Meta信息$file\_[i][j]$，确定Key可能所在的SSTable。
   1. 第0层因为SSTable是直接从Immutable MemTable复制过来的，其中的SSTable之间的键值表示范围可能存在Overlap，所以待查找的Key可能存在于多个SSTable中，只能依次遍历每个SSTable的Meta信息，判断待查找的Key是否在第$j$个SSTable的范围内，即$file\_[0][j].smallest\le Key\le file\_[0][j].largest$ 。
   2. 第1层到第6层，因为SSTable都是通过Compaction得到的，Compaction的时候会对多个SSTable进行多路归并，SSTable之间的键值表示范围不会存在Overlap，所以待查找的Key至多只存在一个SSTable中，可以根据Meta信息中的最大键值做二分查找，最终确定可能的SSTable。

   当确定了Key可能存在的SSTable时，可以通过至多3次IO读写确定Key是否真正存在在SSTable中，及其对应的Value。

   1. 读取BloomFilter Block，根据BloomFilter确定Key是否存在，若存在则进行下一步，否则检查下一个可能的SSTable。
   2. 读取IndexBlock，确定Key所在的DataBlock的索引。
   3. 读取Key所在DataBlock，查找Key对应的Value。若不存在Key，则检查下一个可能的SSTable，否则结束查找，返回对应的Value。

从上述的查找流程发现，原本查找Key所在的一个SSTable，却因为LSM-tree的构造多读了好几个SSTable，才能找到待查找的Key真正所在的位置。根据统计可以知道，基于LSM-tree的DB的读放大平均达到了？倍，其中LevelDB高达？倍。造成读放大的原因是因为Compaction机制的引入，将数据逐渐向高层流动。当查找一个真正存在于高层的Key时，可能每一层都会有一个SSTable的范围包括了这个Key，那么在每一层都需要去检查一下这个Key是否存在在那个SSTable中。

为了解决这个读放大的问题，我们认为数据的流动方向不应该只是从$L_0$到$L_6$的，同时也应该可以从$L_6$流向$L_0$，LSM-tree中的数据应该是随着数据的访问频率而动态分布的。当高层数据最近频繁被访问时，应该将高层数据向低层流动，从而提高读性能。同时，因为原来一次的Compaction的overhead是十分巨大的，所以高层数据向低层流动时不应该再引入较大的overhead。

## Solutions

### 二级缓存替换MemTable，多Immutable MemTable

用LRU2Q来替换MemTable，当数据从2Q淘汰后，将其加入Immutable Memtable。当Immutable Memtable达到阈值时，开始向$L_0$层Dump，在Dump的同时创建一个新的Immutable Memtable用于接收新的被淘汰的数据。

LRU的每个队列用一个TreapTree实现。



### 数据上浮

每层维护一个LRU队列，$L_1$到$L_6$层均通过Rollback操作将SSTable提示至上一层。

类似LRU的MQ变形，对每个Level的SSTable的维护一个LRU队列。

**数据记录：**

为每个文件在内存中维护最近命中次数**Hit**。对每次查找，将最终命中的文件的Hit增加一。

**交换时机：**

当$Hit_{L_{i+1}, j}>=Hit_{L_i,k} \times \alpha$时，将存储在内存中$L_{i+1}$层的SSTable文件$j$和$L_i$层的SSTable文件$k$的逻辑地址（文件指针）交换，并分别在$L_i$层和$L_{i+1}$层标记记录这两个文件。

对$L_i$层来说，若发生了交换操作之前就已有已经标记好的文件$p$，则将新文件$q$与文件$p$合并。需要注意的是，当合并两个文件时，需要注意两个文件的新旧程度，根据文件的新旧程度抛弃旧文件中相同key的数据。文件原来属于层数越低，则文件的新鲜程度越高。

当$L_0$层的SSTable满足条件后，SSTable中的Key直接被提取到MemTable中，同时去除重复的Key。

**Compaction：**

向下Compaction时，选择第$L_i$层频率最低的一个SSTable向下Compaction。

*<u>归并时，按照访问频率排序合并为新的SSTable。</u>*

**懒操作：**

为了不破坏原来的LevelDB中的每层数据结构类型，当文件加入新的一层时应该与该层所有有Overlap的文件进行合并，但是这个合并操作的代价很大，同时若不合并的话，最多对原来的查找文件的数量每层增加一个，同时标记文件相比于原有的文件是旧文件。

向下Compation时，选择频率最低的SSTable文件，合并文件若选择到了标记文件，则取消标记。

### 树自适应变形

**评分函数：**

LevelDB对每层计算一个score，

## 问题







| 号   | 顺序读（scan） | 随机读（read） | 插入（insert） | 更新（update） | 类型                                     |
| ---- | -------------- | :------------- | :------------- | :------------- | ---------------------------------------- |
| 1    | 45%            | 45%            | 5%             | 5%             | 读多写少、顺序读和随机读一样             |
| 2    | 85%            | 5%             | 5%             | 5%             | 读多写少、顺序读多                       |
| 3    | 5%             | 85%            | 5%             | 5%             | 读多写少、随机读多                       |
| 4    | 23%            | 22%            | 45%            | 5%             | 读写一样、顺序读和随机读一样、写新数据多 |
| 5    | 45%            | 5%             | 45%            | 5%             | 读写一样、顺序读多、写新数据多           |
| 6    | 5%             | 45%            | 45%            | 5%             | 读写一样、随机读多、写新数据多           |
| 7    | 22%            | 23%            | 5%             | 45%            | 读写一样、顺序读和随机读一样、更新数据多 |
| 8    | 45%            | 5%             | 5%             | 45%            | 读写一样、顺序读多、更新数据多           |
| 9    | 5%             | 45%            | 5%             | 45%            | 读写一样、随机读多、更新数据多           |
| 10   | 2%             | 3%             | 90%            | 5%             | 读少写多、写新数据多                     |
| 11   | 2%             | 3%             | 5%             | 90%            | 读少写多、更新数据多                     |
| 12   | 2%             | 3%             | 47%            | 48%            | 读少写多、写新数据和更新数据一样         |

