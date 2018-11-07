#include "skiplist.h"

namespace bilsmtree {

SkipList::SkipList() {
  head_ = new ListNode();
  head_->level_ = 0;
  head_->forward_ = new (ListNode*)[MAXLEVEL];
  for (size_t i = 0; i < MAXLEVEL; ++ i)
    head_->forward_[i] = NULL;
  data_size_ = 0;
}

SkipList::~SkipList() {
  delete[] head_;
}

bool SkipList::IsFull() {
  return data_size_ >= ImmutableMemTableConfig::MAXSIZE;
}

Slice SkipList::Find(const Slice& key) {
  ListNode* p = head_;
  for (size_t i = head_->level_ - 1; i >= 0; -- i) {
    while (p->forward_[i].kv_.key_.compare(key) < 0)
      p = p->forward_[i];
  }
  p = p->forward_[0];
  if (p->kv_.key_.compare(key) == 0)
    return p->kv_.value_;
  return NULL;
}

void SkipList::Insert(const KV& kv) {
  ListNode* p = head_;
  ListNode* update_[MAXLEVEL];
  for (size_t i = head_->level_ - 1; i >= 0; -- i) {
    while (p->forward_[i].kv_.key_.compare(kv.key_) < 0)
      p = p->forward_[i];
    update_[i] = p;
  }
  p = p->forward_[0];
  if (p->kv_.key_.compare(kv.key_) == 0) {
    p->kv_.value_ = kv.value_;
  }
  else {
    ListNode* q = new ListNode();
    q->kv_ = kv;
    q->level_ = GenerateLevel();
    q->forward_ = new (ListNode*)[q->level_];
    if (q->level_ > head_->level_) {
      for (size_t i = head_->level_; i < q->level_; ++ i)
        update_[i] = head_;
      head_->level_ = q->level_;
    }
    for (size_t i = 0; i < q->level_; ++ i) {
      q->forward_ = update_[i]->forward_[i];
      update_[i]->forward_[i] = q;
    }
  }
}

void SkipList::Delete(const Slice& key) {
  ListNode* p = head_;
  ListNode* update_[MAXLEVEL];
  for (size_t i = head_->level_ - 1; i >= 0; -- i) {
    while (p->forward_[i].kv_.key_.compare(key) < 0)
      p = p -> forward_[i];
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
  }
}

std::vector<KV> SkipList::GetAll() {
  std::vector<KV> res_;
  for (size_t i = 0; i < data_size_; ++ i)
    res_.push_back(head_[i].kv_);
  return res_;
}

size_t SkipList::GenerateLevel() {
  size_t level_ = 1;
  while ((rand() * 1.0 / INT_MAX) < PROB && level_ < MAXLEVEL) 
    level_ = level_ + 1;
  return level_; 
}

}