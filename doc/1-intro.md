# Title

Towards Read-Optimized Key-value Stores on Flash Storage

# Abstract

键值存储已经在大规模数据存储应用中扮演着十分重要的角色。基于LSM-Tree结构的键值存储在频繁写（write-intensive）的workload上能够取得很好的性能，主要因为是它能够把一系列的随机写转换为一批（batch）的顺序写，并通过Compaction操作形成层次结构。但是，在读写共存的workload或全读的workload上，LSM-Tree需要花费更高的延迟。造成这个问题的主要原因在于LSM-Tree结构中的层次查找机制和Compaction机制，但因为这两个机制难以被改变，所以我们所面临的关键挑战在于如何在已有机制的基础上提出新的策略以提高LSM-Tree的读性能，同时也保持几乎一样的写性能。

本文提出FSLSM-Tree，一种上浮可伸展的日志结构合并树（<u>*F*</u>loating <u>*S*</u>play <u>*LSM-Tree*</u>）。FSLSM-Tree首先允许频繁访问的SSTable从低层移动到高层，以减少查找延迟。其次还支持树为了适应不同的workload的形状，使其性能达到最大化。目的是提高LSM-Tree在读紧张（read-intensive）的workload上的性能，并使LSM-Tree的结构得到最大化使用。为了验证FSLSM-Tree的可行性，我们使用数据库标准测试工具设计了一系列的实验。实验结果表明，相比于具有代表性的方案，FSLSM-Tree能够明显的提高读性能，减少读放大。

# Introduction

随着云计算的快速发展和Web2.0时代的到来，越来越多的数据中心及企业对云服务更加亲睐，线上用户数据正在成指数级的趋势增长[LOCS, LSbM-Tree, SPEICHER, Sherdder]。传统的关系型数据库的缺点被渐渐暴露出来，对于这种超大规模数据的存储以及高并发访问的服务，传统关系型数据库的效率变得很低[NoSQL]。而此时非关系型数据NoSQL逐渐占据了主导地位，被频繁得使用在大数据应用之中。NoSQL简化了传统关系型数据库的设计，去除了传统关系型数据库中的一系列操作，提供了更高的查询性能。

NoSQL包含许多高效的数据结构，如键值存储，图存储，文档存储等，其中键值存储是NoSQL中比较流行的一项技术。键值存储将一个集合的对象或记录，一一映射到对应的唯一的数据，以便于快速的查询。LSM-Tree是键值存储中的一个最常见的数据结构，往往被应用在大规模的数据存储应用中，如Google的BigTable[BigTable]和LevelDB[LevelDB]，Facebook的RocksDB[RocksDB]和Cassandra[Cassandra]，Twitter的HBase[HBase]，Yahoo!的PUNTS[PNUTS]。LSM-Tree的主要思想是将一系列的随机写转换成顺序写。LSM-Tree首先在内存中将更新和插入保存在一个buffer中，当buffer满了以后，再将这一批buffer刷入外部存储设备中，以达到持久化。在外部存储设备中的数据，LSM-Tree用一个多层次的结构来进行管理。当某层中的文件数据达到上限，LSM-Tree将多出的文件合并到下一层中。当LSM-Tree需要从外部存储设备中读取键值对时，LSM-Tree依次从高层往低层查找，直到找到对应的键值对。这种操作的优势在于它延缓了键值对的更新操作，允许同时存在多个相同的键。但因为逻辑上划分的层次结构，不会导致读到旧数据等问题，同时保证了一定的读效率。

近年来，随着每单位体积的flash价格逐渐降低，不少学者都将LSM-Tree和flash结合到一起[HashKV, LOCS, FlashKV, LWC, KVSSD, KVFTL, Wisckey]。在Flash的研究中，冷热数据是一个十分重要的因素，同时冷热数据对基于flash的应用的性能影响也是一个十分重要的因素[H-C in SSD, Identify H-C, HashKV]。对于热的数据我们希望能够尽可能的留在内存或离内存更近的位置，而对于冷的数据则希望被移动到廉价的外部存储设备中[Necessity]。我们观察到在LSM-Tree结构中并没有对冷热数据进行区分，也没有针对冷热数据进行不同的存储方式。同时随着DUMP和Compaction机制的不断进行，内存的数据不断向外部存储设备移动，并且高层的数据也逐渐被往低层移动，对于低层中热数据的读操作的代价逐渐增大。以LSM-Tree结构中最出名的数据库LevelDB为例，最差情况下，查询一个KV需要读取10个文件。同时，我们还观察到LSM-Tree在外存中的结构是正三角形状的，正是因为这种形状才得到这么高的写性能，但也正是因为这种形状才导致了读性能的下降，尤其是在读多写少或全读的情况下。

为了解决上述问题，本文提出FSLSM-Tree，一种上浮可伸展的日志合并树。FSLSM-Tree是在Wisckey[Wisckey]所提出的键值分离策略基础上进一步改进的。我们首先提出上浮机制，它分为两个部分，一个是内存部分，一个是外存部分。内存部分允许数据在准备DUMP到外存时被重新取回到内存buffer中；外存部分允许低层中的文件通过给定的机制上浮到高层。其次我们再提出伸展机制，它允许LSM-Tree不再局限于原来的正三角形形状，可以根据当前的trace特点来自适应调整树的形状，它可以是倒三角形，也可以是矩形或其他的不规则形状。最后，我们将这两种机制结合在一起得到FSLSM-Tree。

为了验证FSLSM-Tree，我们设计了一系列由专业数据库测试工具YCSB[YCSB]生成的读写trace。我们分别和Wisckey、LevelDB进行了比较，Wisckey是一种高效的键值分离存储策略，LevelDB是键值存储中最为流行的数据库。实验结果表明FSLSM-Tree有效的降低了访问延迟和读放大，同时不产生更多的写放大。

文章的主要贡献是：

* 我们提出了一种上浮机制，允许让热数据所在的文件尽可能的向内存靠近。
* 我们提出了一种伸展机制，打破了原有的LSM-Tree的固定形状。
* 我们使用专业测试工具生成的trace和现在具有代表性的方法进行了比较。

# Reference

1. [LOCS]
2. [LSbM-Tree]
3. [SPEICHER]
4. [Sherdder]：Shredder: GPU-Accelerated Incremental Storage and Computation, FAST, 2012
5. [NoSQL]：Scalable SQL and NoSQL Data Stores，SIGMOD Rec., 2011
6. [LevelDB]
7. [BigTable]
8. [RocksDB]
9. [Cassandra]
10. [HBase]
11. [PUNTS]
12. [HashKV]
13. [H-C in SSD]：An Empirical Study of Hot/Cold Data Separation Policies in Solid State Drives (SSDs)
14. [Identify H-C]：Identifying Hot and Cold Data in Main-Memory Databases
15. [Necessity]：On the necessity of hot and cold data identification to reduce the write amplification in flash-based SSDs
16. [LWC]
17. [KVSSD]
18. [KVFTL]
19. [Wisckey]
20. [YCSB]