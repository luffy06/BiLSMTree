
# Structure

[x] - DB

[x] -- CacheServer

[x] --- LRU2Q

[x] ---- BiList

[x] --- SkipList

[x] -- KVServer

[x] --- LSMTree

[x] ---- VisitFrequency

[x] ---- Table

[x] ----- Block

[x] ----- Filter(BloomFilter / CuckooFilter)

[x] ------ Slice

[x] ------ Hash

[x] --- LogManager

[x] - FileSystem

[x] - Flash

# CRITERIA

1. 访问时间
2. Flash擦除次数
3. 写放大
4. 读放大
5. compaction 次数
6. db查找一个key时多查找的次数
7. immutable memtable dump到l0的次数

## OVERHEAD

* 时间
* 空间

**思考好的指标和差的指标**

# TODO

1. DEBUG
2. 检查Immutable MemTable DUMP
3. REDESIGN SKIPLIST GetAll()