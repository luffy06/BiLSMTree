#include "skiplist.h"

namespace bilsmtree {

SkipList::SkipList() {
  head_ = new ListNode();
  head_->level_ = GenerateLevel();
  head_->forward_ = new ListNode*[Config::SkipListConfig::MAXLEVEL];
  for (size_t i = 0; i < head_->level_; ++ i)
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
  ListNode* p = head_;
  assert(head_ != NULL);
  for (int i = static_cast<int>(head_->level_) - 1; i >= 0; -- i) {
    while (p->forward_[i] != NULL && p->forward_[i]->kv_.key_.compare(key) < 0)
      p = p->forward_[i];
  }
  p = p->forward_[0];
  if (p->kv_.key_.compare(key) == 0) {
    value = Slice(p->kv_.value_.data(), p->kv_.value_.size());
    return true;
  }
  return false;
}

void SkipList::Insert(const KV kv) {
  assert(writable_);
  ListNode *p = head_;
  ListNode *update_[Config::SkipListConfig::MAXLEVEL];
  for (int i = static_cast<int>(head_->level_) - 1; i >= 0; -- i) {
    while (p->forward_[i] != NULL && p->forward_[i]->kv_.key_.compare(kv.key_) < 0)
      p = p->forward_[i];
    update_[i] = p;
  }
  p = p->forward_[0];
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
}

void SkipList::Delete(const Slice key) {
  assert(writable_);
  ListNode *p = head_;
  ListNode *update_[Config::SkipListConfig::MAXLEVEL];
  for (int i = static_cast<int>(head_->level_) - 1; i >= 0; -- i) {
    while (p->forward_[i] != NULL && p->forward_[i]->kv_.key_.compare(key) < 0)
      p = p->forward_[i];
    update_[i] = p;
  }
  p = p->forward_[0];
  if (p->kv_.key_.compare(key) == 0) {
    for (size_t i = 0; i < head_->level_; ++ i) {
      if (update_[i]->forward_[i] != p)
        break;
      update_[i]->forward_[i] = p->forward_[i];
    }
    delete p;
    while (head_->level_ > 0 && head_->forward_[head_->level_] == NULL)
      head_->level_ = head_->level_ - 1;
    data_size_ = data_size_ - 1;
  }
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