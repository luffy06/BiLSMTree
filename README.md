# Require

* C++ >= 11
* CMAKE

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

1. 访问时间(R)
2. Flash擦除次数(R)
3. 写放大(R)
4. 读放大(R)
5. compaction次数(R)
6. db查找一个key时多查找的次数(R)
7. immutable memtable dump到l0的次数(R)

## Config

BiLSMTree
LevelDB-KV
LevelDB

## OVERHEAD

* 时间
* 空间

**思考好的指标和差的指标**

# TODO

1. REDESIGN SKIPLIST GetAll()