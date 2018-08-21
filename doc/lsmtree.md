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

LevelDB是一个基于LSM树并使用键值对进行存储的数据库。

## 存储结构

REF[https://github.com/google/leveldb/blob/master/doc/impl.md]

LevelDB在内存中主要的数据存储结构为两个有序跳表（skiplist），一个为可变表，称为MemTable，另一个是只读的不可变表，称为Immutable MemTable；在磁盘中的主要存储结构为一系列的有序字符表，称为SSTable，一个MANIFEST文件和一些辅助文件。

每个SSTable存储了一系列根据键值排序的条目，每个条目要么是键值对应的value，要么是键值对应的删除标志。这些SSTable被分为7个等级，自顶向下依次用$L_0$到$L_6$来表示。除了$L_0$层以外，其他层中不包含重复的键，$L_0$层中的数据从Immutable MemTable中得到。每一层都有一个最大SSTable的数量，且随着层数增大依次按一定比率增大。

MANIFEST文件罗列了当前层所包含的SSTable文件，当前层所表示的键值范围和其他的重要的数据。

## 插入操作

每当需要插入一个键值对到LevelDB时，首先待插入的键值插入磁盘中的日志文件中，同时也插入到内存中的可变表中。当可变表满了以后，LevelDB使用一个新的可变表，同时之前的可变表转换成了只读的不可变表，随后通过一个压缩操作将它流向磁盘，生成一个新的SSTable文件放在$L_0$级，之前的可变表和日志文件随后被丢弃。

## 压缩操作

当$L_i$层的SSTable数量达到上限时，就需要向下进行压缩。首先从$L_i$层中选择一个SSTable，同时在$L_{i+1}$层中选择所有与其表示的范围有交集的SSTable。将这些SSTable按照键值范围进行合并，按照SSTable的上限大小划分成新的多个SSTable，最终插入到$L_{i +1}$层中。

需要注意的是$L_0$到$L_1$的压缩操作与其他层之间的压缩操作有所不同，因为在$L_0$中，自己的SSTable之间表示的键值范围就有所相交，所以从$L_0$层中可能会选择出超过一个的SSTable。

对$L_i$层，上一次的压缩操作是的最后一个键值将被记录，用于在下一次压缩时，选择第一个从这个键值开始的文件。

# Related Work

## WiscKey

将Key和value的location（用偏移量来表示offset）存在LSM树中，而真实的value存储在SSD中，为了高效的进行垃圾回收将key和value同时存储在SSD中。

查找数据时，现在LSM树中查找到key和location，再根据location去找对应的value。

插入数据时，将value直接插入log文件末尾，将key和value的位置一起插入LSM树。

删除数据时，直接删除LSM树中对应的key。

日志文件中有效的数据在LSM树中都有对应的key，无效的数据将在垃圾回收时被清除。

垃圾回收时从tail到head开始进行垃圾回收，每次对读取到的数据根据对应的key在LSM树中查找，若是有效数据，则将该条数据重新插入head之后，更新tail指针。为了避免crash发生时的数据不同步，将tail也存储在LSM树中。

## LOCS

LOCS系统分为软件层和硬件层。软件层分为4个层次：LevelDB层，scheduler层，存储API层，SSD驱动层。不同于传统的系统设计，LOCS系统多了一个scheduler层，它的主要任务是为LevelDB发来的请求在SDF上进行规划。

修改LevelDB以支持SDF。

### Round-Robin Dispatching（RR）

对所有的channel，以一个循环的顺序依次安排**写操作**。

当出现一些读操作和擦除操作是，每个channel的队列将不平衡。

### Least Weighted-Queue-Length Write Dispatching（LWQL）

通过一个队列的带权长度表来预测某个队列执行所安排的请求的延迟。首先为三种请求（读、写、擦除）设置不同的权重，然后根据下面公式得出队列的带权长度。
$$
Length_{weight}=\sum^N_1 W_i\times Size_i
$$
每次将写操作插入到当前权重最小的队列中。

## Elastic BF

Elastic BF又称为Elastic Bloom Filter。Bloom Filter用于可以快速的检查请求的数据是否存在与SSTable中，从而减少了额外的I/O消耗。一般的BF在不同的level中都使用的同样的配置，也无法动态调整，Elastic BF是一种更细粒度的，更灵活的filter。

# 引用

[SB-Tree] Patrick E. O'Neil, "The SB-tree: An index-sequential structure for high-performance sequential access", Acta Informatica 29, 241-265 (1992).