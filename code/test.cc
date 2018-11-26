#include <cassert>
#include <ctime>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "db.h"

const size_t TotalKV = 40;
const size_t KeySize = 100;
const size_t ValueSize = 1024;

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

std::vector<bilsmtree::KV> GenerateRandomKVPairs() {
  std::vector<bilsmtree::KV> kvs;
  for (size_t i = 0; i < TotalKV; ++ i) {
    std::string key = "";
    std::string value = "";
    for (size_t j = 0; j < KeySize; ++ j)
      key = key + GenerateCharacter();
    for (size_t j = 0; j < ValueSize; ++ j)
      value = value + GenerateCharacter();
    kvs.push_back(bilsmtree::KV(key, value));
  }
  std::cout << "GENERATE SUCCESS" << std::endl;
  return kvs;
}

void TestFlash(const std::vector<bilsmtree::KV>& data) {
  std::cout << seg << "TEST Flash" << seg <<  std::endl;
  std::string msg = "SUCCESS";
  bilsmtree::FlashResult *flashresult = new bilsmtree::FlashResult();
  bilsmtree::Flash *flash = new bilsmtree::Flash(flashresult);
  for (size_t i = 0; i < data.size(); ++ i) {
    // std::cout << "TEST " << i << std::endl << data[i].value_ << std::endl;
    size_t lba = i;
    flash->Write(lba, data[i].value_.data());
  }
  for (size_t i = 0; i < data.size(); ++ i) {
    size_t lba = i;
    char *db_data = flash->Read(lba);
    if (data[i].value_.ToString() != std::string(db_data)) {
      // std::cout << "Second " << i << "\nDB DATA" << std::endl << db_data << std::endl << "DATA" << std::endl << data[i].value_ << std::endl;
      msg = "Read error! Data doesn't match";
    }
  }
  for (size_t i = 0; i < data.size(); ++ i) {
    // std::cout << "TEST " << i << std::endl << data[i].key_ << std::endl;
    size_t lba = i;
    flash->Write(lba, data[i].key_.data());
  }
  for (size_t i = 0; i < data.size(); ++ i) {
    size_t lba = i;
    char *db_data = flash->Read(lba);
    if (data[i].key_.ToString() != std::string(db_data)) {
      // std::cout << "First " << i << "\nDB DATA" << std::endl << db_data << std::endl << "DATA" << std::endl << data[i].key_ << std::endl;
      msg = "Read error! Data doesn't match";
    }
  }
  delete flash;
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestFileSystem(const std::vector<bilsmtree::KV>& data) {
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
      filesystem->Write(file_number, data[i].value_.ToString().data(), data[i].value_.size());
    }
    filesystem->Seek(file_number, 0);
    for (size_t j = 0; j < 3; ++ j) {
      filesystem->Write(file_number, data[i].key_.ToString().data(), data[i].key_.size());
    }
    filesystem->Close(file_number);
  }
  std::cout << "TEST Read" << std::endl;
  for (size_t i = 0; i < data.size(); ++ i) {
    std::string filename = "test_" + bilsmtree::Util::IntToString(i);
    size_t file_number = filesystem->Open(filename, bilsmtree::Config::FileSystemConfig::READ_OPTION);
    std::string fdata = filesystem->Read(file_number, data[i].key_.size());
    if (fdata != data[i].key_.ToString())
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

void TestBiList(const std::vector<bilsmtree::KV>& data) {
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

void TestSkipList(const std::vector<bilsmtree::KV>& kv_data) {
  std::cout << seg << "TEST SkipList" << seg << std::endl;
  std::string msg = "SUCCESS";  
  // std::vector<bilsmtree::KV> kv_data;
  bilsmtree::SkipList *skiplist = new bilsmtree::SkipList();
  // for (size_t i = 1; i <= 20; ++ i) {
  //   bilsmtree::KV kv = bilsmtree::KV(std::string(i, '@'), std::string(i, '@'));
  //   kv_data.push_back(kv);
  // }
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
  for (int i = kv_data.size() - 1; i >= 0; -- i) {
    bilsmtree::KV kv = kv_data[i];
    std::string v = std::string(i, '$');
    kv.value_ = bilsmtree::Slice(v.data(), v.size());
    skiplist->Insert(kv);
  }
  s_data = skiplist->GetAll();
  for (size_t i = 0; i < s_data.size(); ++ i) 
    std::cout << s_data[i].key_.ToString() << std::endl;
  std::cout << (skiplist->IsFull() ? "FULL" : "NOT FULL") << std::endl;
  std::cout << seg << std::endl;

  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestLRU2Q(const std::vector<bilsmtree::KV>& data) {
  std::cout << seg << "TEST LRU2Q" << seg << std::endl;
  std::string msg = "SUCCESS";  
  std::vector<bilsmtree::KV> kv_data;
  bilsmtree::LRU2Q *lru2q = new bilsmtree::LRU2Q();
  for (size_t i = 1; i <= 20; ++ i) {
    bilsmtree::KV kv = bilsmtree::KV(std::string(i, '@'), std::string(i, '@'));
    kv_data.push_back(kv);
  }

  std::cout << "TEST Put" << std::endl;
  for (size_t i = 0; i < kv_data.size(); ++ i) {
    bilsmtree::KV pop_kv;
    if (lru2q->Put(kv_data[i], pop_kv)) {
      msg = "Put error";
    }
  }

  std::vector<bilsmtree::KV> res = lru2q->GetAll();
  std::cout << "Data Size:" << res.size() << std::endl;
  for (size_t i = 0; i < res.size(); ++ i) 
    std::cout << res[i].key_.ToString() << std::endl;

  std::cout << "TEST Get 1" << std::endl;
  for (size_t i = 0; i < kv_data.size() / 2; ++ i) {
    bilsmtree::Slice value;
    if (!lru2q->Get(kv_data[i].key_, value)) {
      msg = "Value doesn't exist";
    }
    else if (value.compare(kv_data[i].value_) != 0) {
      msg = "Value doesn't match";
    }
  }

  res = lru2q->GetAll();
  std::cout << "Data Size:" << res.size() << std::endl;
  for (size_t i = 0; i < res.size(); ++ i)
    std::cout << res[i].key_.ToString() << std::endl;

  std::cout << "TEST Get 2" << std::endl;
  for (size_t i = 0; i < kv_data.size(); i += 2) {
    bilsmtree::Slice value;
    if (!lru2q->Get(kv_data[i].key_, value)) {
      msg = "Value doesn't exist";
    }
    else if (value.compare(kv_data[i].value_) != 0) {
      msg = "Value doesn't match";
    }
  }

  res = lru2q->GetAll();
  std::cout << "Data Size:" << res.size() << std::endl;
  for (size_t i = 0; i < res.size(); ++ i)
    std::cout << res[i].key_.ToString() << std::endl;

  delete lru2q;
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestCuckooFilter(const std::vector<bilsmtree::KV>& data) {
  std::cout << seg << "TEST CuckooFilter" << seg << std::endl;
  std::string msg = "SUCCESS";
  std::cout << "TEST Construction" << std::endl;
  std::vector<bilsmtree::Slice> res;
  for (size_t i = 0; i < data.size(); ++ i)
    res.push_back(data[i].key_);
  bilsmtree::CuckooFilter *cuckoofilter = new bilsmtree::CuckooFilter(res);

  std::cout << "TEST KeyMatch" << std::endl;
  for (size_t i = 0; i < data.size(); ++ i) {
    if (!cuckoofilter->KeyMatch(data[i].key_)) {
      msg = "Key doesn't exist";
      break;
    }
  }

  // std::cout << cuckoofilter->ToString() << std::endl << std::endl;
  std::cout << "TEST Diff" << std::endl;
  res.clear();
  for (size_t i = 0; i < data.size(); i = i + 2)
    res.push_back(data[i].key_);
  bilsmtree::CuckooFilter *cuckoofilter2 = new bilsmtree::CuckooFilter(res);
  // std::cout << cuckoofilter2->ToString() << std::endl << std::endl;
  std::cout << "Ready Diff" << std::endl;
  cuckoofilter->Diff(cuckoofilter2);
  // std::cout << cuckoofilter->ToString() << std::endl << std::endl;
  for (size_t i = 0; i < data.size(); ++ i) {
    if (i % 2 == 0 && cuckoofilter->KeyMatch(data[i].key_)) {
      std::cout << data[i].key_.ToString() << std::endl;
      msg = "Diff error! Key delete failed";
      break;
    }
    else if (i % 2 != 0 && !cuckoofilter->KeyMatch(data[i].key_)) {
      std::cout << data[i].key_.ToString() << std::endl;
      msg = "Diff error! Key doesn't exist";
      break;
    }
  }

  delete cuckoofilter;
  delete cuckoofilter2;
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestBloomFilter(const std::vector<bilsmtree::KV>& data) {
  std::cout << seg << "TEST BloomFilter" << seg << std::endl;
  std::string msg = "SUCCESS";
  std::cout << "TEST Construction" << std::endl;
  std::vector<bilsmtree::Slice> res;
  for (size_t i = 0; i < data.size(); ++ i)
    res.push_back(data[i].key_);
  bilsmtree::BloomFilter *blooomfilter = new bilsmtree::BloomFilter(res);

  std::cout << "TEST KeyMatch" << std::endl;
  for (size_t i = 0; i < data.size(); ++ i) {
    if (!blooomfilter->KeyMatch(data[i].key_)) {
      msg = "Key doesn't exist";
      break;
    }
  }

  delete blooomfilter;
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

void TestDB(const std::vector<bilsmtree::KV>& data) {
  std::cout << seg << "TEST DB" << seg << std::endl;
  std::string msg = "SUCCESS";
  bilsmtree::DB *db = new bilsmtree::DB();
  std::cout << "TEST Put" << std::endl;
  for (size_t i = 0; i < data.size(); ++ i) {
    std::cout << "Put " << i << std::endl;
    db->Put(data[i].key_.ToString(), data[i].value_.ToString());
  }
  size_t t;
  std::cin >> t;
  std::cout << "TEST Get" << std::endl;
  for (size_t i = 0; i < data.size(); ++ i) {
    std::string db_value;
    size_t r = rand() % 2;
    std::cout << "RUN " << i << "\tOp:" << (r == 0 ? "Put" : "Get") << std::endl;
    if (r == 0) {
      db->Put(data[i].key_.ToString(), data[i].value_.ToString());
    }
    else {
      if (!db->Get(data[i].key_.ToString(), db_value)) {
        std::cout << std::endl << data[i].key_.ToString() << " Cannot Get" << std::endl;
        msg = "Key doesn't exist";
        break;
      }
      else if (db_value != data[i].value_.ToString()) {
        msg = "Value  doesn't match DBValue:";
        break;
      }
    }
  }
  delete db;
  std::cout << seg << "TEST RESULT: " << msg << seg << std::endl;
}

int main() {
  size_t seed = 1000000007;
  srand(seed);
  // std::vector<bilsmtree::KV> data = GenerateRandomKVPairs();
  std::vector<bilsmtree::KV> small_data;
  for (size_t i = 1; i <= 2000; ++ i) {
    // std::string key = std::string(i, '@');
    std::string key = bilsmtree::Util::IntToString(i);
    std::string value = std::string(i, '@');
    bilsmtree::KV kv = bilsmtree::KV(key, value);
    // std::cout << kv.key_.ToString() << std::endl;
    small_data.push_back(kv);
  }
  std::cout << "GENERATE SUCCESS" << std::endl;
  size_t k;
  std::cin >> k;
  // TestFlash(data);
  // TestFileSystem(data);
  // TestBiList(data);
  // TestSkipList(small_data);
  // TestLRU2Q(data);
  // TestCuckooFilter(small_data);
  // TestBloomFilter(small_data);
  // for (size_t i = 0; i < 5; ++ i)
  TestDB(small_data);
  return 0;
}