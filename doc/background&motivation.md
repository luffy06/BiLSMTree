# Background

## LevelDB

### 简介

LevelDB is a fast key-value storage library written at Google that provides an ordered mapping from string keys to string values【LevelDB】.LevelDB is an open source key-value store that originated
from Google’s BigTable 【Bigtable】它具有很高的随机写（45.0MB/s）、顺序写（62.7MB/s和顺序读性能（232.2MB/s），但随机读性能一般【LevelDB - Performance】。LevelDB在外存存储时使用了LSM-Tree【LSM-tree】的结构，对索引变更进行延迟及批量处理，并通过多路归并的方法高效的将数据更新迁移到磁盘，降低索引插入的开销。

### 结构

![LevelDBStructure](/Users/wsy/Documents/ProgramDoc/BiLSMTree/doc/pic/leveldb.png)

【图1：LevelDB结构】

图1是LevelDB的整体结构。

The main data structures in LevelDB are an on-disk log file, two in-memory sorted skiplists (memtable and immutable memtable), and seven levels ($L_0$ to $L_6$) of on-disk Sorted String Table (SSTable) files.【WiscKey】

LevelDB initially stores inserted key-value pairs in a log file and the in-memory memtable. Once the memtable is full, LevelDB switches to a new memtable and log file to handle further inserts from the user; in the background, the previous memtable is converted into an immutable memtable, and a compaction thread then flushes it to disk, generating a new SSTable file at level 0 ($L_0$) with a rough size of 2MB; the previous log file is subsequently discarded. 【WiscKey】

当MemTable写满后，LevelDB将它被转换成一个不可写的Immutable MemTable，等待着被移动到外存，并创建一个新的空的MemTable接受新的输入。【https://github.com/google/leveldb/blob/master/doc/impl.md （Level 0）】

SSTable是LevelDB在外存中主要的数据组织形式，每个SSTable文件可以划分成4个区域：数据块区域，索引块区域，Meta块区域，footer块区域。数据块区域由许多数据块构成，每个数据块包含许多个键值对数据。索引块区域包含一个索引块，它存储了每个数据块的最大值、在文件中的偏移量等相关信息，偏于快速定位到数据所在的数据块。Meta块区域可以用于对LevelDB进行功能扩展，如添加过滤器以加速查找过程。footer块区域存储了每个区域在SSTable文件中的相应位置。【https://github.com/google/leveldb/blob/master/doc/table_format.md】

The youngest level, Level 0, is produced by writing the Immutable MemTable from main memory to the disk. 【LOCS】When the size of level L exceeds its limit, we compact it in a background thread. The compaction picks a file from level L and all overlapping files from the next level L+1. Note that if a level-L file overlaps only part of a level-(L+1) file, the entire file at level-(L+1) is used as an input to the compaction and will be discarded after the compaction. Aside: because level-0 is special (files in it may overlap each other), we treat compactions from level-0 to level-1 specially: a level-0 compaction may pick more than one level-0 file in case some of these files overlap each other.【https://github.com/google/leveldb/blob/master/doc/impl.md（Compactions）】

Thus SSTables in Level 0 could contain overlapping keys. However, in other levels the key range of SSTables are non-overlapping. Each level has a limit on the maximum number of SSTables, or equivalently, on the total amount of data because each SSTable has a fixed size in a level. The limit grows at an exponential rate with the level number. For example, the maximum amount of data in Level 1 will not exceed 10 MB, and it will not exceed 100 MB for Level 2. 【LOCS】

## CuckooFilter介绍及结合

BloomFilter具有以下两点不足：

1. 具有一定的错误率。将不存在的元素返回存在。
2. 无法删除元素。

也有很多对BloomFilter的改进：

- 引入计数。extend Bloom filters to allow deletions. A counting Bloom filter uses an array of counters in place of an array of bits. 【摘自CuckooFilter中对Counting BloomFilter的引用】
- 分成小块。do not support deletion, but provide better spatial locality on lookups. A blocked Bloom filter
  consists of an array of small Bloom filters, each fitting in one CPU cache line. 【摘自CuckooFilter中对Blocked BloomFilter的引用】
- Hash tables using d-left hashing [19] store fingerprints for stored items. These filters delete items by removing their fingerprint.  【摘自CuckooFilter中对d-left Counting BloomFilter的引用】

### CuckooFilter的介绍

Cuckoo filters support adding and removing items dynamically while achieving even higher performance than Bloom filters. 【CuckooFilter】

![cuckoofilter](/Users/wsy/Documents/ProgramDoc/BiLSMTree/doc/pic/cuckoofilter.png)

【图2：用于理解CuckooFilter，不使用该图】

CuckooFilter使用的Hash方法是CuckooHash【CuckooHash】，A basic cuckoo hash table consists of an array of buckets where each item has two candidate buckets determined by hash functions h1(x) and
h2(x).  【CuckooFilter】

A basic cuckoo hash table consists of an array of buckets where each item has two candidate buckets determined by hash functions h1(x) and h2(x).  If either of x’s two buckets is empty, the algorithm inserts x to that free bucket and the insertion completes. If neither bucket has space, as is the case in this example, the item selects one of the candidate buckets, kicks out the existing item (in this case “a”) and re-inserts this victim item to its own alternate location. 【CuckooFilter】The lookup of the element can be performed using the same two hash values.  【PA CuckooFilter】

### CuckooFilter的结合

用CuckooFilter来表示一个SSTable中都包含哪些Key。当因为Overlap而要删除SSTable中的某些Key时，仅仅删除CuckooFilter中的Key，而不重新组织DataBlock。

具体见：外存：数据上浮，替换Filter --> 1. 数据如何上浮？ --> 具体操作

## Flash

Flash是一种长寿命的非易失性（在断电情况下仍能保持所存储的数据信息）的存储器。普通的Flash芯片是平面结构，数据只能前后左右移动。

3D NAND Flash通过堆叠栅极层的结构提高了比特密度，可以实现数据在三维空间中的存储和传递，大幅提高了存储设备的存储能力。3D NAND Flash基本存储单元是Page , 擦除操作的基本单位是块。每一Page的有效容量是512字节的倍数。由于每个块的擦除时间几乎相同，块的容量将直接决定擦除性能。

# Motivation

目标：**基于Flash的LevelDB，在不影响原有写性能的基础上优化读放大问题**。

## LevelDB查找过程

在LevelDB查找一个Key的步骤主要可以分为**在内存查找**和**在外存查找**两步：

1. 在内存中，首先在MemTable中查找，若没有找到，则检查Immutable MemTable是否存在。若存在，在Immutable MemTable中查找。
2. 若在内存中均未找到，则在外存中查找。因为数据存在于外存中的SSTable文件中，所以首先要确定数据所在的SSTable文件，其次再从对应的SSTable文件中读取具体的数据。
   1. 内存中维护了所有SSTable的Meta信息，包括SSTable的最小值、最大值等。除$L_0$层外，其他层均可以通过二分查找确定Key可能存在的SSTable。$L_0$层因为存在Overlap，所以只能依次遍历判断。
   2. 确定了Key可能存在的SSTable时，需要读取SSTable文件来判断Key是否真正存在于这个SSTable中，这个步骤可以通过至多3次IO读写来实现。
      1. 读取BloomFilter Block，根据BloomFilter确定Key是否存在，若存在则进行下一步，否则检查下一个可能的SSTable。
      2. 读取IndexBlock，确定Key所在的DataBlock的索引。
      3. 读取Key所在DataBlock，查找Key对应的Value。若不存在Key，则检查下一个可能的SSTable，否则结束查找，返回对应的Value。

从上述的查找流程发现，原本查找Key所在的一个SSTable，却因为特殊的索引结构多读了好几个SSTable，才能找到待查找的Key真正所在的位置。根据统计可以知道，基于LevelDB的读放大平均达到了<u>？</u>倍。造成读放大的原因是因为Compaction机制的引入，将数据逐渐向低层流动。当查找一个真正存在于低层的Key时，因为可能每一层都会有一个SSTable的范围包括了这个Key，所以在每一层都需要进行至多3次IO去检查一下这个Key是否存在在那层的SSTable中。

为了解决这个读放大的问题，我们认为数据的流动方向不应该只是从$L_0$到$L_6$的，同时也应该可以**从$L_6$流向$L_0$，**同时外存中的数据不应该仅仅按照新旧程度从上至下分布，还应该**随着数据的访问频率而动态分布**。当低层数据最近频繁被访问时，应该将其向高层流动，从而提高读性能。同时，因为原来一次的Compaction的overhead是十分巨大的，所以**数据上浮时不应该再引入较大的overhead**。

# REF

[LSM-tree]：The Log-Structured Merge-Tree

[GoogleWeb]：https://github.com/google/leveldb

[BloomFilter]：

[CuckooFilter]：Cuckoo FIlter：Practically Better Than Bloom

[LOCS]：An Efficient Design and Implementation of LSM-Tree based Key-Value Store on Open-Channel SSD 

[BigTable]：F. Chang, J. Dean, S. Ghemawat, W. C. Hsieh, D. A. Wallach, M. Burrows, T. Chandra, A. Fikes, and R. E. Gruber. Bigtable: A distributed storage system for structured data. ACM Trans. Comput. Syst., 26(2):4:1–4:26, June 2008 

[WiscKey]：

[PA CuckooFilter]：Minseok Kwon, Vijay Shankar, Pedro Reviriego:Position-aware cuckoo filters. ANCS 2018: 151-153

[Counting BloomFilter]：L. Fan, P. Cao, J. Almeida, and A. Z. Broder. Summary cache: A scalable wide-area Web cache sharing protocol. In Proc. ACM SIGCOMM, Vancouver, BC, Canada, Sept. 1998. 

[Blocked BloomFilter]：F. Putze, P. Sanders, and S. Johannes. Cache-, hash- and spaceefficient bloom filters. In Experimental Algorithms, pages 108–121. Springer Berlin / Heidelberg, 2007. 

[d-left Counting BloomFilter]：F. Bonomi, M. Mitzenmacher, R. Panigrahy, S. Singh, and
G. Varghese. An improved construction for counting bloom filters. In 14th Annual European Symposium on Algorithms, LNCS 4168, pages 684–695, 2006. 

[CuckooHash]：R. Pagh and F. Rodler. Cuckoo hashing. Journal of Algorithms,51(2):122–144, May 2004. 