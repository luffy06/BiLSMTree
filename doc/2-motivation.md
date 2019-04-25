# Background

## LevelDB

LevelDB[LevelDB]是现在十分流行的基于LSM-Tree的键值存储数据库之一，它受到了BigTable[BigTable]的启发，将一批随机的写转换为顺序写，从而获得了很高的写性能。LevelDB支持Snapshot、Forward and backward iteration等其他常见的数据特性。

![LevelDBStructure](/Users/wsy/Documents/ProgramDoc/BiLSMTree/doc/pic/leveldb.png)

【图1：LevelDB结构】

图1是LevelDB的整体结构。LevelDB将内部存储空间划分为两个有跳表（SkipList）[Skiplist]实现的MemTable，将外部存储空间划分为一系列存储了键值对及它们索引信息的SSTable（Sorted  String Table）文件和一些辅助文件，如Log文件，存储了SSTabel的Meta信息的Manifest文件等等。这些SSTable文件在逻辑上被分为k层结构，分别用$L_0,\ldots ,L_k$表示，默认k=7。每层SSTable文件的个数是有上限的，一旦超过上限，则会触发Compaction操作。$L_0$层的文件个数上限默认是4个文件，剩下其他的$L_i$层的文件个数上限是$10^i$个文件。除了$L_0$层外，其他层中的SSTable之间是不存在键值范围重叠的，这样在进行查找的时候可以通过二分查找等高效的查找方法确定包含指定键的SSTable文件。$L_0$层的文件由一个列表维护，列表中越靠近头部的文件中的数据越新，所以每次新加入的文件会插入到列表的头部。在$L_0$层中进行查找时，只能通过顺序查找的方法依次检查每个文件。

当插入或更新（删除被当作是一种特殊的更新操作，在具体操作时会设置一个删除标记）一个键值对时，首先会被插入到MemTable中。MemTable中不存在相同的键值对，更新操作将直接就地更新。当MemTable的容量达到设定上限时，LevelDB将它被转换成一个Immutable MemTable，等待着被刷到外存，同时创建一个新的空MemTable接收新的操作。因为LevelDB增加了版本控制，所以Flush操作可以在后台进行， 完成后将新的版本与当前版本进行合并即可。新刷到外部存储设备的Immutable MemTable会被转换成SSTable，直接插入到$L_0$层。当$L_0$层中的文件数据达到上限时，触发Compaction操作。Compaction操作会首先从$L_0$层选择一个目标文件，再从$L_0$层和$L_1$层选择与目标文件存在键值范围重叠的所有文件。这些文件中的键值对将会全部被读入到内存中，通过多路合并形成一系列新的SSTable，再将它们插入$L_1$层。随后，在$L_1$层或更低层进行相同的Compaction操作。一般来说，$L_i$层的Compaction操作在$L_i$层选择了目标文件后，只需要在$L_{i+1}$层选择存在键值范围重叠的文件即可，因为$L_i$层的所有SSTable文件不存在键值范围重叠，但$L_0$层除外，所以$L_0$层还需要在同层选择键值范围重叠的文件。

当查询一个键值对时，LevelDB首先在内存中的MemTable和Immutable MemTable进行查询，若查询到就直接返回结果，否则需要到外部存储设备中进行查找。在外部存储设备中的查找是按照层级进行查找的，从$L_0$层依次查找到$L_6$层，一旦找到就返回结果，这样保证了查找的键值对一定是最新的。

## 读放大

读放大是LSM-Tree结构的数据库中的一个主要问题。它主要有3个方面的因素引起：第一个是LevelDB的多层次结构导致了每次查询一个键值对时需要查询多个层次中的SSTable文件。最坏情况下，LevelDB需要在$L_0$层中检查8个文件，其他每层各检查一个文件，总共是14个文件[Wisckey]。第二个原因是SSTable的文件组织结构所导致的。每个SSTable文件可以划分成4个区域：数据块区域，索引块区域，Meta块区域，footer块区域。数据块区域由许多数据块构成，每个数据块包含许多个键值对数据。索引块区域包含一个索引块，它存储了每个数据块的最大值、在文件中的偏移量等相关信息，偏于快速定位到数据所在的数据块。Meta块区域可以用于对LevelDB进行功能扩展，如添加过滤器以加速查找过程。footer块区域存储了每个区域在SSTable文件中的相应位置。所以当检查SSTable时，需要先读取footer区域，获取Meta块区域的位置和索引块区域的位置，如若有过滤器，则先读取过滤器部分，判断键是否存在；如若没有，则直接读取索引块区域，再根据索引读取对应的数据块，判断是否存储键值对。最坏情况下，需要读取一个footer块区域、一个Meta块区域，一个索引块区域和一个数据块区域。对于查找1KB的键值对，LevelDB一般需要读取16KB的索引块，4KB的Meta块区域，4KB数据块，footer块很小可以忽略不计，故总共需要读取24KB的数据[Wisckey]。第三个原因是随着数据的不断写入，LevelDB会发生Compaction操作。而Compaction操作中，选择完待合并的文件后，要将这些文件的键值对都读取到内存，这些都属于额外的读。通过静态实验分析，在一般的workload中，每层合并时涉及约5个SSTable[LOCS]，一个SSTable默认是2MB，所以每次Compaction要读取约10MB的数据。

# Motivation

从上述的分析可以得出，查找Key时有两个原因导致读放大，一个是LSM-Tree的层次结构，另一个是SSTable的文件组织形式，同时额外的一个导致读放大的原因是Compaction机制。在LSM-Tree的层级结构的基础中，数据通过Compaction机制被渐渐流动到低层，在查找低层数据时会导致巨大的读放大。

为了解决这个读放大的问题，我们打算仿照Compaction机制，设计一个新的机制，改变数据的流动方向。我们认为数据的流动方向不应该只是从$L_0$到$L_6$的，同时也应该可以从$L_6$流向$L_0$，使得能够有效的减少对低层数据的读放大问题。但考虑到LSM-Tree的层次结构是上窄下宽，若频繁向上移动数据，必然会引发多次Compaction操作，这样反而不会减少读放大，甚至会增加读放大。所以我们可以适时的打破LSM-Tree的层次结构的形状，根据不同的Trace适当的改变形状，如在读多写少的情况下，可以形成类似上宽下窄的形状。

# Reference

1. [LevelDB]：http://code.google.com/p/leveldb.
2. [Skiplist]：Pugh W. Skip lists: a probabilistic alternative to balanced trees[J]. Communications of the ACM, 1990, 33(6).
3. [BigTable]：Chang F, Dean J, Ghemawat S, et al. Bigtable: A distributed storage system for structured data[J]. ACM Transactions on Computer Systems (TOCS), 2008, 26(2): 4.
4. [Wisckey]：Lu L, Pillai T S, Arpaci-Dusseau A C, et al. WiscKey: separating keys from values in SSD-conscious storage[C]//Proceedings of the 14th Usenix Conference on File and Storage Technologies. USENIX Association, 2016: 133-148.
5. [LOCS]：Wang P, Sun G, Jiang S, et al. An efficient design and implementation of LSM-tree based key-value store on open-channel SSD[C]//Proceedings of the Ninth European Conference on Computer Systems. ACM, 2014: 16.