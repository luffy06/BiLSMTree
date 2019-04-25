# Title

Towards Read-Optimized Key-value Stores with Flexible Structure

# Abstract

键值存储已经在大规模数据存储应用中扮演着十分重要的角色。基于LSM-Tree结构的键值存储在频繁写（write-intensive）的workload上能够取得很好的性能，主要因为是它能够把一系列的随机写转换为一批（batch）的顺序写，并通过Compaction操作形成层次结构。但是，在读写共存的workload或全读的workload上，LSM-Tree需要花费更高的延迟。造成这个问题的主要原因在于LSM-Tree结构中的层次查找机制和Compaction机制，但因为这两个机制难以被改变，所以我们所面临的关键挑战在于如何在已有机制的基础上提出新的策略以提高LSM-Tree的读性能，同时也保持几乎一样的写性能。

本文提出FSLSM-Tree，一种上浮可伸展的日志结构合并树（<u>*F*</u>loating <u>*S*</u>play <u>*LSM-Tree*</u>）。FSLSM-Tree首先允许频繁访问的SSTable从低层移动到高层，以减少查找延迟。其次还支持树为了适应不同的workload的形状，使其性能达到最大化。目的是提高LSM-Tree在读紧张（read-intensive）的workload上的性能，并使LSM-Tree的结构得到最大化使用。为了验证FSLSM-Tree的可行性，我们使用数据库标准测试工具设计了一系列的实验。实验结果表明，相比于具有代表性的方案，FSLSM-Tree能够明显的提高读性能，减少读放大。

# Introduction

随着云计算的快速发展和Web2.0时代的到来，越来越多的数据中心及企业对云服务更加亲睐，线上用户数据正在成指数级的趋势增长[LOCS, LSbM-Tree, SPEICHER, Sherdder]。传统的关系型数据库的缺点被渐渐暴露出来，对于这种超大规模数据的存储以及高并发访问的服务，传统关系型数据库的效率变得很低[NoSQL]。而此时非关系型数据NoSQL逐渐占据了主导地位，被频繁得使用在大数据应用之中。NoSQL简化了传统关系型数据库的设计，去除了传统关系型数据库中的一系列操作，提供了更高的查询性能。

NoSQL包含许多高效的数据结构，如键值存储，图存储，文档存储等，其中键值存储是NoSQL中比较流行的一项技术。键值存储将一个集合的对象或记录，一一映射到对应的唯一的数据，以便于快速的查询。LSM-Tree[LSM-Tree]是键值存储中的一个最常见的数据结构，往往被应用在大规模的数据存储应用中，如Google的BigTable[BigTable]和LevelDB[LevelDB]，Facebook的RocksDB[RocksDB]和Cassandra[Cassandra]，Twitter的HBase[HBase]，Yahoo!的PUNTS[PNUTS]。LSM-Tree的主要思想是将一系列的随机写转换成顺序写。LSM-Tree首先在内存中将更新和插入保存在一个buffer中，当buffer满了以后，再将这一批buffer刷入外部存储设备中，以达到持久化。在外部存储设备中的数据，LSM-Tree用一个多层次的结构来进行管理。当某层中的文件数据达到上限，LSM-Tree将多出的文件合并到下一层中。当LSM-Tree需要从外部存储设备中读取键值对时，LSM-Tree依次从高层往低层查找，直到找到对应的键值对。这种操作的优势在于它延缓了键值对的更新操作，允许同时存在多个相同的键。但因为逻辑上划分的层次结构，不会导致读到旧数据等问题，同时保证了一定的读效率。

近年来，随着每单位体积的flash价格逐渐降低，不少学者都将LSM-Tree和flash结合到一起[HashKV, LOCS, FlashKV, LWC, KVSSD, KVFTL, Wisckey]。在Flash的研究中，冷热数据是一个十分重要的因素，同时冷热数据对基于flash的应用的性能影响也是一个十分重要的因素[H-C in SSD, Identify H-C, HashKV]。对于热的数据我们希望能够尽可能的留在内存或离内存更近的位置，而对于冷的数据则希望被移动到廉价的外部存储设备中[Necessity]。我们观察到在LSM-Tree结构中并没有对冷热数据进行区分，也没有针对冷热数据进行不同的存储方式。同时随着DUMP和Compaction机制的不断进行，内存的数据不断向外部存储设备移动，并且高层的数据也逐渐被往低层移动。同时，我们还观察到LSM-Tree在外存中的结构是正立的三角形状的，除非人为改变，否则是不会变化的。在真实世界中的workload往往都是读紧张的，Facebook曾指出在他们的Get/Set比例能够达到30:1[Workload]。在这些机制的前提下，对于低层的热数据的读操作的代价将会逐渐增大。以LSM-Tree结构中最出名的数据库LevelDB为例，最差情况下，查询一个KV需要读取10个文件。

为了提高LSM-Tree的读写性能，不少学者都提出了相应的解决方案，针对LSM-Tree的结构进行了重新设计：对LSM-Tree的Compaction机制进行改进，Dostoevsky[Dostoevsky]和LWC-tree；也有对LSM-Tree中的键值存储进行分离存储，Wisckey[Wisckey]和HashKV[HashKV]；还有学者为LSM-Tree中每层增加一个Buffer，设计了Skip-tree[Skip-tree]结构；还有学者为LSM-Tree添加了额外的前缀树，提出LSM-trie[LSM-trie]，这些方法都显著的提升了LSM-Tree的写性能，但对读性能的提升效果不是那么明显。有一些学者也为LSM-Tree的读机制进行了优化，ElasticBF[ElasticBF]设计了新的BloomFilter机制，SifrDB[SifrDB]提出森林结构的LSM-Tree，这些方法改进了LSM-Tree的读性能，但并没有从本质上去解决LSM-Tree结构中所引入的读性能降低问题。

本文提出FSLSM-Tree，一种上浮可伸展的日志合并树。FSLSM-Tree是在Wisckey[Wisckey]所提出的键值分离策略基础上进一步改进的。我们首先提出上浮机制，它分为两个部分，一个是内存部分，一个是外存部分。内存部分允许数据在准备DUMP到外存时被重新取回到内存buffer中；外存部分允许低层中的文件通过给定的机制上浮到高层。其次我们再提出伸展机制，它允许LSM-Tree不再局限于原来的正三角形形状，可以根据当前的trace特点来自适应调整树的形状，它可以是倒三角形，也可以是矩形或其他的不规则形状。最后，我们将这两种机制结合在一起得到FSLSM-Tree。

为了验证FSLSM-Tree，我们设计了一系列由专业数据库测试工具YCSB[YCSB]生成的读写trace。我们分别和Wisckey、LevelDB进行了比较，Wisckey是一种高效的键值分离存储策略，LevelDB是键值存储中最为流行的数据库。实验结果表明FSLSM-Tree有效的降低了访问延迟和读放大，同时不产生更多的写放大。

文章的主要贡献是：

* 我们提出了一种上浮机制，允许让热数据所在的文件尽可能的向内存靠近。
* 我们提出了一种伸展机制，打破了原有的LSM-Tree的固定形状。
* 我们使用专业测试工具生成的trace和现在具有代表性的方法进行了比较。

# Reference

1. [LOCS]：Wang P, Sun G, Jiang S, et al. An efficient design and implementation of LSM-tree based key-value store on open-channel SSD[C]//Proceedings of the Ninth European Conference on Computer Systems. ACM, 2014: 16.
2. [LSbM-Tree]：Teng D, Guo L, Lee R, et al. LSbM-tree: Re-enabling buffer caching in data management for mixed reads and writes[C]//2017 IEEE 37th International Conference on Distributed Computing Systems (ICDCS). IEEE, 2017: 68-79.
3. [SPEICHER]：Bailleu M, Thalheim J, Bhatotia P, et al. {SPEICHER}: Securing LSM-based Key-Value Stores using Shielded Execution[C]//17th {USENIX} Conference on File and Storage Technologies ({FAST} 19). 2019: 173-190.
4. [Sherdder]：Bhatotia P, Rodrigues R, Verma A. Shredder: GPU-accelerated incremental storage and computation[C]//FAST. 2012, 14: 14.
5. [NoSQL]：Cattell R. Scalable SQL and NoSQL data stores[J]. Acm Sigmod Record, 2011, 39(4): 12-27.
6. [LSM-Tree]：O’Neil P, Cheng E, Gawlick D, et al. The log-structured merge-tree (LSM-tree)[J]. Acta Informatica, 1996, 33(4): 351-385.
7. [LevelDB]：http://code.google.com/p/leveldb.
8. [BigTable]：Chang F, Dean J, Ghemawat S, et al. Bigtable: A distributed storage system for structured data[J]. ACM Transactions on Computer Systems (TOCS), 2008, 26(2): 4.
9. [RocksDB]：http://rocksdb.org/.
10. [Cassandra]：http://cassandra.apache.org/.
11. [HBase]：http://hbase.apache.org/.
12. [PUNTS]：Cooper B F, Ramakrishnan R, Srivastava U, et al. PNUTS: Yahoo!'s hosted data serving platform[J]. Proceedings of the VLDB Endowment, 2008, 1(2): 1277-1288.
13. [Workload]：Atikoglu B, Xu Y, Frachtenberg E, et al. Workload analysis of a large-scale key-value store[C]//ACM SIGMETRICS Performance Evaluation Review. ACM, 2012, 40(1): 53-64.
14. [HashKV]：Chan H H W, Liang C J M, Li Y, et al. HashKV: Enabling Efficient Updates in {KV} Storage via Hashing[C]//2018 {USENIX} Annual Technical Conference ({USENIX}{ATC} 18). 2018: 1007-1019.
15. [FlashKV]：Zhang J, Lu Y, Shu J, et al. FlashKV: Accelerating KV performance with open-channel SSDs[J]. ACM Transactions on Embedded Computing Systems (TECS), 2017, 16(5s): 139.
16. [H-C in SSD]：Lee J, Kim J S. An empirical study of hot/cold data separation policies in solid state drives (SSDs)[C]//Proceedings of the 6th International Systems and Storage Conference. ACM, 2013: 12.
17. [Identify H-C]：Levandoski J J, Larson P Å, Stoica R. Identifying hot and cold data in main-memory databases[C]//2013 IEEE 29th International Conference on Data Engineering (ICDE). IEEE, 2013: 26-37.
18. [Necessity]：Van Houdt B. On the necessity of hot and cold data identification to reduce the write amplification in flash-based SSDs[J]. Performance Evaluation, 2014, 82: 1-14.
19. [LWC]：Yao T, Wan J, Huang P, et al. Building efficient key-value stores via a lightweight compaction tree[J]. ACM Transactions on Storage (TOS), 2017, 13(4): 29.
20. [KVSSD]：Wu S M, Lin K H, Chang L P. KVSSD: Close integration of LSM trees and flash translation layer for write-efficient KV store[C]//2018 Design, Automation & Test in Europe Conference & Exhibition (DATE). IEEE, 2018: 563-568.
21. [KVFTL]：Chen Y T, Yang M C, Chang Y H, et al. Co-Optimizing Storage Space Utilization and Performance for Key-Value Solid State Drives[J]. IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems, 2019, 38(1): 29-42.
22. [Dostoevsky]：Dayan N, Idreos S. Dostoevsky: Better space-time trade-offs for LSM-tree based key-value stores via adaptive removal of superfluous merging[C]//Proceedings of the 2018 International Conference on Management of Data. ACM, 2018: 505-520.
23. [Skip-tree]：Yue Y, He B, Li Y, et al. Building an efficient put-intensive key-value store with skip-tree[J]. IEEE Transactions on Parallel and Distributed Systems, 2017, 28(4): 961-973.
24. [LSM-trie]：Wu X, Xu Y, Shao Z, et al. LSM-trie: An LSM-tree-based ultra-large key-value store for small data items[C]//2015 {USENIX} Annual Technical Conference ({USENIX}{ATC} 15). 2015: 71-82.
25. [ElasticBF]：Zhang Y, Li Y, Guo F, et al. ElasticBF: Fine-grained and Elastic Bloom Filter Towards Efficient Read for LSM-tree-based {KV} Stores[C]//10th {USENIX} Workshop on Hot Topics in Storage and File Systems (HotStorage 18). 2018.
26. [SifrDB]：Mei F, Cao Q, Jiang H, et al. SifrDB: A Unified Solution for Write-Optimized Key-Value Stores in Large Datacenter[J]. ACM SoCC, 2018: 477-489.
27. [Wisckey]：Lu L, Pillai T S, Arpaci-Dusseau A C, et al. WiscKey: separating keys from values in SSD-conscious storage[C]//Proceedings of the 14th Usenix Conference on File and Storage Technologies. USENIX Association, 2016: 133-148.
28. [YCSB]：Cooper B F, Silberstein A, Tam E, et al. Benchmarking cloud serving systems with YCSB[C]//Proceedings of the 1st ACM symposium on Cloud computing. ACM, 2010: 143-154.