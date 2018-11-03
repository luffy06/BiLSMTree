
# Structure

[ ] - DB
[x] -- CacheServer
[x] --- LRU2Q
[x] ---- BiList
[x] --- SkipList
[ ] -- KVServer
[ ] --- LSMTree
[x] ---- VisitFrequency
[ ] ---- Table
[x] ----- Block
[x] ----- Filter(BloomFilter / CuckooFilter)
[x] ------ Slice
[x] ------ Hash
[x] --- LogManager
[ ] - FileSystem
[x] - Flash

# TODO

1. Table::Table(const std::string& filename);
2. void LSMTree::CompactList(size_t level);
3. std::vector<Table*> LSMTree::MergeTables(const std::vector<Table*> tables);
4. void FileSystem::Seek(const int& file_number, const size_t& offset);
5. Flash change algorithm.