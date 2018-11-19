#include "skiplist.h"

namespace bilsmtree {

SkipList::SkipList() {
  head_ = new ListNode();
  head_->level_ = GenerateLevel();
  head_->forward_ = new ListNode*[Config::SkipListConfig::MAXLEVEL];
  for (size_t i = 0; i < Config::SkipListConfig::MAXLEVEL; ++ i)
    head_->forward_[i] = NULL;
  data_size_ = 0;
  writable_ = true;
}

SkipList::~SkipList() {
  while (head_ != NULL) {
    ListNode *p = head_->forward_[0];
    delete head_;
    head_ = p;
  }
}

bool SkipList::IsFull() {
  return data_size_ >= Config::ImmutableMemTableConfig::MAXSIZE;
}

bool SkipList::Find(const Slice key, Slice& value) {
  // std::cout << "SkipList::Find" << std::endl;
  ListNode* p = head_;
  assert(head_ != NULL);
  for (int i = static_cast<int>(head_->level_) - 1; i >= 0; -- i) {
    while (p->forward_[i] != NULL && p->forward_[i]->kv_.key_.compare(key) < 0)
      p = p->forward_[i];
  }
  p = p->forward_[0];
  if (p != NULL && p->kv_.key_.compare(key) == 0) {
    value = Slice(p->kv_.value_.data(), p->kv_.value_.size());
    // std::cout << "SkipList::Find True" << std::endl;
    return true;
  }
  // std::cout << "SkipList::Find False" << std::endl;
  return false;
}

void SkipList::Insert(const KV kv) {
  assert(writable_);
  assert(head_ != NULL);
  // std::cout << "SkipList::Insert" << std::endl;
  ListNode *p = head_;
  ListNode *update_[Config::SkipListConfig::MAXLEVEL];
  for (int i = static_cast<int>(head_->level_) - 1; i >= 0; -- i) {
    // std::cout << "Search Level:" << i << std::endl;
    while (p->forward_[i] != NULL && p->forward_[i]->kv_.key_.compare(kv.key_) < 0)
      p = p->forward_[i];
    // std::cout << "Search Level End" << std::endl;
    update_[i] = p;
  }
  p = p->forward_[0];
  // std::cout << "In Bottom" << std::endl;
  if (p != NULL && p->kv_.key_.compare(kv.key_) == 0) {
    p->kv_.value_ = Slice(kv.value_.data(), kv.value_.size());
  }
  else {
    assert(!IsFull());
    ListNode* q = new ListNode();
    q->kv_.key_ = Slice(kv.key_.data(), kv.key_.size());
    q->kv_.value_ = Slice(kv.value_.data(), kv.value_.size());
    q->level_ = GenerateLevel();
    q->forward_ = new ListNode*[q->level_];
    if (q->level_ > head_->level_) {
      for (size_t i = head_->level_; i < q->level_; ++ i)
        update_[i] = head_;
      head_->level_ = q->level_;
    }
    for (size_t i = 0; i < q->level_; ++ i) {
      q->forward_[i] = update_[i]->forward_[i];
      update_[i]->forward_[i] = q;
    }
    data_size_ = data_size_ + 1;
  }
  // std::cout << "SkipList::Insert Success" << std::endl;
}

void SkipList::DisableWrite() {
  writable_ = false;
}

size_t SkipList::GenerateLevel() {
  size_t level_ = 1;
  while ((rand() * 1.0 / 10000000) < Config::SkipListConfig::PROB && level_ < Config::SkipListConfig::MAXLEVEL) 
    level_ = level_ + 1;
  return level_; 
}

}