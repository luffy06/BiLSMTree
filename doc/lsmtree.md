# LSMTree

一个LSM-Tree可以由多个组件$C_0,C_1,\ldots, C_k$构成，其中$C_0$存在于***内存***之中，剩余的$C_1,\ldots,C_k$存在于***外存***之中。每个组件都是由一个数据结构来维护，一般来说$C_0$由<u>**2-3树，AVL树**</u>等构成，$C_1,\ldots,C_k$由<u>**满节点的B树**</u>构成，同时$C_1$到$C_k$的规模逐渐增大。

## 二组件的LSM-Tree

最简单的LSM-Tree包含两个组件，常驻内存的称为$C_0$树，常驻外存的称为$C_1$树。

![figure1](./pic/simplelsmtree.png)

【图1 两个部件构成的LSM-Tree】

$C_1$树的结构类似B树，不同的是每个节点都是**全满的**，类似SB树[SB-Tree]。

当有一个新数据被生成后，将进行如下步骤的操作：

1. 将用于恢复这个插入操作的日志记录写入日志文件。
2. 将该数据的索引插入$C_0$树。

当$C_0$树达到阈值时，需要将$C_0$树中的一部分数据迁移到$C_1$树中。这个操作叫做**Rolling Merge**，它删除了$C_0$树中连续的片段，并将这些片段合并到$C_1$树中。

# LevelDB

LevelDB是Google开源的持久化KV数据库，具有很高的随机写、顺序读/写性能，但随机读性能一般。LevelDB应用了LSM树的结构，对索引变更进行延迟及批量处理，并通过类归并排序的方法高效的将数据更新迁移到磁盘，降低索引插入的开销。

## 存储结构

REF[https://github.com/google/leveldb/blob/master/doc/impl.md]

LevelDB在内存中主要的数据存储结构为两个有序跳表（skiplist），一个为可变表，称为MemTable，另一个是只读的不可变表，称为Immutable MemTable；在磁盘中的主要存储结构为一系列的有序字符表，称为SSTable，一个MANIFEST文件，一个CURRENT文件以及一些log文件等辅助文件。这些文件之间的组织形式如图2。

![structure](./pic/structure.png)

【图2 存储模型快照】

### SSTable

每个SSTable存储了一系列根据key的值排序的条目，每个条目要么是key对应的value，要么是key对应的删除标志。这些SSTable被分为7个等级，自顶向下依次用$L_0$到$L_6$来表示。除了$L_0$层以外，其他层中不包含重复的key，$L_0$层中的数据从Immutable MemTable中得到。每一层都有一个最大SSTable的数量，且随着层数增大依次按一定比率增大。

每个SSTable文件的上限是2MB，由若干个4KB大小的block组成，最后一个block用于存储每个block的index信息和第一个key的值。同一个block中的key可以共享前缀，每个key只需存储自己唯一的后缀即可。若仅有部分key可以共享前缀，这部分key和其它key之间插入“reset”标识。

### MANIFEST文件

MANIFEST文件罗列了当前层所包含的SSTable文件，当前层所表示的键值范围和其他的重要的数据。

### CURRENT文件

由于Compaction是在后台进行的，LevelDB将会维护多个版本的数据，CURRENT文件指出当前哪些文件是最新的文件。

## 读写操作

### 写操作

每当需要插入一个键值对到LevelDB时，

1. 将待插入的键值插入磁盘中的日志文件中；
2. 同时也插入到内存中的可变表中；
3. 当内存中的可变表满了以后，LevelDB使用一个新的可变表，同时之前的可变表转换成了只读的不可变表，随后通过一个压缩操作将它流向磁盘中的$L_0$级：生成一个新的SSTable文件，将不可变表中的数据拷贝至SSTable，将这个SSTable放入$L_0$级，之前的可变表和日志文件随后被丢弃。

### 读操作

1. 在内存中依次查找MemTable，Immutable MemTable
2. 若配置了Cache，查找Cache
3. 根据MANIFEST索引文件，在磁盘中查找SSTable。

## 压缩操作

为了加快读取速度，LevelDB利用压缩操作对已有的键值对记录进行整理压缩，删除一些不再有效的键值对数据，减少数据规模，减少文件数量。

LevelDB中的压缩操作分为3种类型：

* minor compaction：将immutable的数据导出到SSTable文件中。
* major compaction：合并不同层级的SSTable文件。
* full compaction：合并所有的SSTable。

### minor compaction

当内存中MemTable大小达到阈值时，需要将MemTable转换成Immutable MemTable，再将数据迁移到磁盘中。因为Immutable MemTable是一个SkipList，其中的数据根据Key有序排列的，所以只需依次写入Level 0中的SSTable，并建立index数据即可。

### major compaction

当$L_i$层的SSTable数量达到上限时，就需要向下进行压缩。首先从$L_i$层中选择一个SSTable，同时在$L_{i+1}$层中选择所有与其表示的范围有交集的SSTable。将这些SSTable按照键值范围进行合并，按照SSTable的上限大小划分成新的多个SSTable，最终插入到$L_{i +1}$层中。

需要注意的是$L_0$到$L_1$的压缩操作与其他层之间的压缩操作有所不同，因为在$L_0$中，自己的SSTable之间表示的键值范围就有所相交，所以从$L_0$层中可能会选择出超过一个的SSTable。

对$L_i$层，上一次的压缩操作是的最后一个键值将被记录，用于在下一次压缩时，选择第一个从这个键值开始的文件。

## 版本控制

当执行一次compaction或创建一个Iterator时，LevelDB将在当前版本的基础上创建一个新的版本。

LevelDB用VersionSet来管理所有的Version，用VersionEdit来表示Version之间的变化，相当于增量。

`Version0 + VersionEdit --> Version1`

# Ideas

## Motivation

基于Flash的LevelDB，在不影响原有的Compaction速度的基础上优化**读放大问题**。

* 原有的查找方法
  * 查找MemTable，再查找Immutable MemTable。
  * 依次查找$L_i$层的SSTable文件，$L_0$层与其他层需要区分：
    * $L_0$层顺序遍历每个SSTable文件。
    * 其他层中可以根据每个SSTable对应的FileMetaData记录的最大键值做二分查找。
  * 找到潜在的SSTable文件后，将index_block内的内容加载入Cache。

* BloomFilter：用k个哈希函数和一个m位数组表示包含n个数据的集合，支持快速查询一个数据是否属于这个集合。

## FrequencyLevel

**代价函数：**

$f_{i,j}$，第$i$层的第$j$个SSTable文件的访问频率。
$$
E[Extra\_IO]=\sum_i \sum_j f_{i,j}\times i +
$$
**分配层数：**



## RollBack

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

**懒操作（？）：**

为了不破坏原来的LevelDB中的每层数据结构类型，当文件加入新的一层时应该与该层所有有Overlap的文件进行合并，但是这个合并操作的代价很大，同时若不合并的话，最多对原来的查找文件的数量每层增加一个，同时标记文件相比于原有的文件是旧文件。

向下Compation时，选择频率最低的SSTable文件，合并文件若选择到了标记文件，则取消标记。

## LocalStorage

**flash存储**

因为一个SSTable分为Data区域、BloomFilter区域、Index区域、Footer区域。

设定一部分Block用于存储所有SSTable的BloomFilter区域，一部分Block用于存储所有SSTable的Index区域，剩下的Block用于存储所有SSTable的Data区域。

index block存储data block的信息时，将原来的offset信息替换为block num和page num，便于查找。

将最近频繁访问的index block存储在同一个page中，同时设置block cache，将index block预取到block cache。每次查找index的时候首先在block cache中查找是否用对应的index，若有则可直接读data block。

## 问题

**Hit**超出存储范围：（每层设着Hit阈值，达到阈值提升到上一层）

磁盘读写次数未减少，增加。（LRU-MQ）

## 难点

数据何时载入内存，载入多少？

如何快速确定哪有包含指定Key的SSTable文件？（用原有的方法？）

ROLLBACK时机？哪些文件ROLLBACK？

是否需要替换上层文件？

# 讨论

* 算法：向上Compaction，利用Lazy操作，延迟Compaction，等到实在需要Compaction时再进行Compaction。
* 列出存在的难点、问题。
* 针对大多数情况：
  * 顺序读顺序写
  * 顺序读随机写
  * 随机读顺序写
  * 随机读随机写
* **单个SSTable的大小？一个page若能放下多个SSTable，将哪些SSTable放在一起？一个page若放不下一个SSTable，将SSTable的哪部分放在一起？SSTable的Block大小？**
  * Flash中Block大小16MB，包含1024个Page，即每个Page大小16KB。
  * SSTable大小可调，一般为2MB。
  * 一个SSTable需要128个Page来存储。
* **Immutable MemTable向下Compact时，有选择的向下Compact。类似LRU-2。**

# 引用

[SB-Tree] Patrick E. O'Neil, "The SB-tree: An index-sequential structure for high-performance sequential access", Acta Informatica 29, 241-265 (1992).