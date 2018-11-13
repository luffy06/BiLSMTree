#include <cassert>
#include <ctime>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "db.h"

const size_t TotalKV = 400;
const size_t KeySize = 100;
const size_t ValueSize = 1024;

typedef std::pair<std::string, std::string> PP;
std::string seg(15, '#');

std::string GenerateCharacter() {
  std::stringstream ss;
  std::string d;
  size_t type = rand() % 3;
  size_t offset = 0;
  if (type) offset = rand() % 26;
  else offset = rand() % 10;
  if (type == 0)
    ss << static_cast<char>('0' + offset);
  else if (type == 1)
    ss << static_cast<char>('a' + offset);
  else
    ss << static_cast<char>('A' + offset);
  ss >> d;
  return d;
}

std::vector<PP> GenerateRandomKVPairs() {
  std::vector<std::pair<std::string, std::string>> kvs;
  for (size_t i = 0; i < TotalKV; ++ i) {
    std::string key = "";
    std::string value = "";
    for (size_t j = 0; j < KeySize; ++ j)
      key = key + GenerateCharacter();
    for (size_t j = 0; j < ValueSize; ++ j)
      value = value + GenerateCharacter();
    kvs.push_back(std::make_pair(key, value));
  }
  std::cout << "GENERATE SUCCESS" << std::endl;
  return kvs;
}

void TestFlash(const std::vector<PP>& data) {
  std::cout << seg << "TEST Flash" << seg <<  std::endl;
  std::string msg = "SUCCESS";
  bilsmtree::FlashResult *flashresult = new bilsmtree::FlashResult();
  bilsmtree::Flash *flash = new bilsmtree::Flash(flashresult);
  for (size_t i = 0; i < data.size(); ++ i) {
    // std::cout << "TEST " << i << std::endl << data[i].second << std::endl;
    size_t lba = i;
    flash->Write(lba, data[i].second.data());
  }
  for (size_t i = 0; i < data.size(); ++ i) {
    size_t lba = i;
    char *db_data = flash->Read(lba);
    if (data[i].second != std::string(db_data)) {
      // std::cout << "Second " << i << "\nDB DATA" << std::endl << db_data << std::endl << "DATA" << std::endl << data[i].second << std::endl;
      msg = "Read error! Data doesn't match";
    }
  }
  for (size_t i = 0; i < data.size(); ++ i) {
    // std::cout << "TEST " << i << std::endl << data[i].first << std::endl;
    size_t lba = i;
    flash->Write(lba, data[i].first.data());
  }
  for (size_t i = 0; i < data.size(); ++ i) {
    size_t lba = i;
    char *db_data = flash->Read(lba);
    if (data[i].first != std::string(db_data)) {
      // std::cout << "First " << i << "\nDB DATA" << std::endl << db_data << std::endl << "DATA" << std::endl << data[i].first << std::endl;
      msg = "Read error! Data doesn't match";
    }
  }
  delete flash;
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestFileSystem(const std::vector<PP>& data) {
  std::cout << seg << "TEST FileSystem" << seg << std::endl;
  std::string msg = "SUCCESS";
  bilsmtree::FlashResult *flashresult = new bilsmtree::FlashResult();
  bilsmtree::FileSystem *filesystem = new bilsmtree::FileSystem(flashresult);
  std::vector<size_t> file_numbers;
  std::cout << "TEST Write" << std::endl;
  for (size_t i = 0; i < data.size(); ++ i) {
    std::string filename = "test_" + bilsmtree::Util::IntToString(i);
    size_t file_number = filesystem->Open(filename, bilsmtree::Config::FileSystemConfig::APPEND_OPTION);
    file_numbers.push_back(file_number);
    for (size_t j = 0; j < 3; ++ j) {
      filesystem->Write(file_number, data[i].second, data[i].second.size());
    }
    filesystem->Seek(file_number, 0);
    for (size_t j = 0; j < 3; ++ j) {
      filesystem->Write(file_number, data[i].first, data[i].first.size());
    }
    filesystem->Close(file_number);
  }
  std::cout << "TEST Read" << std::endl;
  for (size_t i = 0; i < data.size(); ++ i) {
    std::string filename = "test_" + bilsmtree::Util::IntToString(i);
    size_t file_number = filesystem->Open(filename, bilsmtree::Config::FileSystemConfig::READ_OPTION);
    std::string fdata = filesystem->Read(file_number, data[i].first.size());
    if (fdata != data[i].first)
      msg = "Read error! Data doesn't match";
    filesystem->Close(file_number);
  }
  for (size_t i = 0; i < data.size(); ++ i) {
    std::string filename = "test_" + bilsmtree::Util::IntToString(i);
    filesystem->Delete(filename);
  }
  delete filesystem;
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestBiList(const std::vector<PP>& data) {
  std::cout << seg << "TEST BiList" << seg << std::endl;
  std::string msg = "SUCCESS";
  bilsmtree::BiList *bilist = new bilsmtree::BiList(10);
  std::vector<bilsmtree::KV> kv_data;
  for (size_t i = 1; i <= 10; ++ i) {
    bilsmtree::KV kv = bilsmtree::KV(std::string(i, '@'), std::string(i, '@'));
    kv_data.push_back(kv);
  }
  std::cout << "TEST Append & Insert" << std::endl;
  for (size_t i = 0; i < 10; ++ i) {
    bilsmtree::KV pop_kv;
    if (i < 5) {
      if (bilist->Append(kv_data[i], pop_kv))
        msg = "Append error! Pop data";
    }
    else {
      if (bilist->Insert(kv_data[i], pop_kv))
        msg = "Insert error! Pop data";
    }
  }
  bilist->Show();
  std::cout << seg << std::endl;

  std::cout << "TEST Set" << std::endl;
  for (size_t i = 0; i < 10; ++ i)
    bilist->Set(i + 1, kv_data[10 - i - 1]);
  bilist->Show();
  std::cout << seg << std::endl;

  std::cout << "TEST Append" << std::endl;
  for (size_t i = 0; i < 10; ++ i) {
    bilsmtree::KV pop_kv;
    if (bilist->Append(kv_data[i], pop_kv))
      std::cout << "POP:" << pop_kv.key_.ToString() << std::endl;
    else 
      msg = "Append error! Pop data";
  }
  bilist->Show();
  std::cout << seg << std::endl;

  std::cout << "TEST Insert" << std::endl;
  for (size_t i = 0; i < 10; ++ i) {
    bilsmtree::KV pop_kv;
    if (bilist->Insert(kv_data[i], pop_kv))
      std::cout << "POP:" << pop_kv.key_.ToString() << std::endl;
    else 
      msg = "Insert error! Pop data";
  }
  bilist->Show();
  std::cout << seg << std::endl;

  std::cout << "TEST MoveToHead" << std::endl;
  for (size_t i = 0; i < 5; ++ i) {
    size_t tail = bilist->Tail();
    bilist->MoveToHead(tail);
  }

  bilist->Show();
  std::cout << seg << std::endl;

  std::cout << "TEST Delete" << std::endl;
  for (size_t i = 0; i < 10; ++ i) {
    bilsmtree::KV kv = bilist->Delete(i + 1);
    std::cout << "Delete:" << kv.key_.ToString() << std::endl;
  }
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestSkipList(const std::vector<PP>& data) {
  std::cout << seg << "TEST SkipList" << seg << std::endl;
  std::string msg = "SUCCESS";  
  std::vector<bilsmtree::KV> kv_data;
  bilsmtree::SkipList *skiplist = new bilsmtree::SkipList();
  for (size_t i = 1; i <= 20; ++ i) {
    bilsmtree::KV kv = bilsmtree::KV(std::string(i, '@'), std::string(i, '@'));
    kv_data.push_back(kv);
  }
  std::cout << "TEST Insert" << std::endl;
  for (size_t i = 0; i < kv_data.size(); ++ i)
    skiplist->Insert(kv_data[i]);
  std::vector<bilsmtree::KV> s_data = skiplist->GetAll();
  for (size_t i = 0; i < s_data.size(); ++ i) 
    std::cout << s_data[i].key_.ToString() << std::endl;
  std::cout << (skiplist->IsFull() ? "FULL" : "NOT FULL") << std::endl;
  std::cout << seg << std::endl;

  std::cout << "TEST Find" << std::endl;
  for (size_t i = 0; i < kv_data.size(); ++ i) {
    bilsmtree::Slice value;
    if (!skiplist->Find(kv_data[i].key_, value))
      msg = "Key doesn't exist";
    else if (value.compare(kv_data[i].value_) != 0)
      msg = "Value doesn't match";
  }
  std::cout << seg << std::endl;

  std::cout << "TEST Update" << std::endl;
  for (size_t i = kv_data.size() - 1; i >= 0; -- i) {
    bilsmtree::KV kv = kv_data[i];
    kv.value_ = bilsmtree::Slice(std::string(i, '$'));
    skiplist->Insert(kv);
  }
  s_data = skiplist->GetAll();
  for (size_t i = 0; i < s_data.size(); ++ i) 
    std::cout << s_data[i].key_.ToString() << std::endl;
  std::cout << (skiplist->IsFull() ? "FULL" : "NOT FULL") << std::endl;
  std::cout << seg << std::endl;

  std::cout << "TEST Delete" << std::endl;
  for (size_t i = 0; i < kv_data.size(); i = i + 2)
    skiplist->Delete(kv_data[i].key_);
  s_data = skiplist->GetAll();
  for (size_t i = 0; i < s_data.size(); ++ i) 
    std::cout << s_data[i].key_.ToString() << std::endl;
  std::cout << seg << std::endl;

  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestCuckooFilter(const std::vector<PP>& data) {
  std::cout << seg << "TEST CuckooFilter" << seg << std::endl;
  std::string msg = "SUCCESS";  
  
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestBloomFilter(const std::vector<PP>& data) {
  std::cout << seg << "TEST BloomFilter" << seg << std::endl;
  std::string msg = "SUCCESS";  
  
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestVisitFrequency(const std::vector<PP>& data) {
  std::cout << seg << "TEST VisitFrequency" << seg << std::endl;
  std::string msg = "SUCCESS";  
  
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestTable(const std::vector<PP>& data) {
  std::cout << seg << "TEST Table" << seg << std::endl;
  std::string msg = "SUCCESS";  
  
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestTableIterator(const std::vector<PP>& data) {
  std::cout << seg << "TEST TableIterator" << seg << std::endl;
  std::string msg = "SUCCESS";  
  
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestLRU2Q(const std::vector<PP>& data) {
  std::cout << seg << "TEST LRU2Q" << seg << std::endl;
  std::string msg = "SUCCESS";  
  
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestLSMTree(const std::vector<PP>& data) {
  std::cout << seg << "TEST LSMTree" << seg << std::endl;
  std::string msg = "SUCCESS";  
  
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestLogManager(const std::vector<PP>& data) {
  std::cout << seg << "TEST LogManager" << seg << std::endl;
  std::string msg = "SUCCESS";  
  
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestKVServer(const std::vector<PP>& data) {
  std::cout << seg << "TEST KVServer" << seg << std::endl;
  std::string msg = "SUCCESS";  
  
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestCacheServer(const std::vector<PP>& data) {
  std::cout << seg << "TEST CacheServer" << seg << std::endl;
  std::string msg = "SUCCESS";
  
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestDB(const std::vector<PP>& data) {
  std::cout << seg << "TEST DB" << seg << std::endl;
  std::string msg = "SUCCESS";
  bilsmtree::DB *db = new bilsmtree::DB();
  for (size_t i = 0; i < data.size(); ++ i) {
    // std::cout << "Key" << std::endl << data[i].first << std::endl << "Value" << std::endl << data[i].second << std::endl;
    db->Put(data[i].first, data[i].second);
  }
  for (size_t i = 0; i < data.size(); ++ i) {
    // std::cout << "Validate " << i << "\t" << data[i].first << std::endl;
    std::string db_value;
    if (!db->Get(data[i].first, db_value)) {
      msg = "Key:" + data[i].first + "\tValue:" + data[i].second + " doesn't exist";
    }
    else if (db_value != data[i].second) {
      msg = "Key:" + data[i].first + "\tValue:" + data[i].second + " doesn't match DBValue:" + db_value;
    }
    else {
      std::cout << "Find Target" << std::endl;
    }
  }
  delete db;
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

int main() {
  size_t seed = 1000000007;
  srand(seed);
  std::vector<PP> data = GenerateRandomKVPairs();
  // TestFlash(data);
  // TestFileSystem(data);
  // TestBiList(data);
  // TestSkipList(data);
  TestDB(data);
  return 0;
}
