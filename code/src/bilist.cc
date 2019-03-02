#include "bilist.h"

namespace bilsmtree {

BiList::BiList(size_t size, size_t numb) {
  Init(size, numb);
}

BiList::~BiList() {
  Init(max_size_, max_numb_);
}

bool BiList::IsFull() {
  if (free_.empty())
    return true;
  if (data_size_ >= max_size_)
    return true;
  return false;
}

KV BiList::DropHead() {
  assert(data_[head_].next_ != -1);
  KV kv = Delete(data_[head_].next_);
  return kv;
}

std::vector<KV> BiList::DropAll() {
  std::vector<KV> res;
  int p = data_[head_].next_;
  while (p != -1) {
    res.push_back(data_[p].kv_);
    p = data_[p].next_;
  }
  Init(max_size_, max_numb_);
  return res;
}

void BiList::Append(const KV kv) {
  assert(!IsFull());
  if (ExistAndUpdate(kv) != -1)
    return ;
  size_t pos = free_.front();
  free_.pop();
  data_size_ = data_size_ + kv.size();
  data_numb_ = data_numb_ + 1;
  data_[pos].kv_ = kv;
  data_[pos].prev_ = tail_;
  data_[pos].next_ = data_[tail_].next_;
  data_[tail_].next_ = pos;
  tail_ = pos;
  assert(data_[head_].prev_ == -1);
}

void BiList::Insert(const KV kv) {
  assert(!IsFull());
  if (ExistAndUpdate(kv) != -1)
    return ;
  size_t pos = free_.front();
  free_.pop();
  data_size_ = data_size_ + kv.size();
  data_numb_ = data_numb_ + 1;
  data_[pos].kv_ = kv;
  data_[pos].prev_ = head_;
  data_[pos].next_ = data_[head_].next_;
  if (data_[head_].next_ != -1)
    data_[data_[head_].next_].prev_ = pos;
  data_[head_].next_ = pos;
  if (tail_ == head_)
    tail_ = pos;
  assert(data_[head_].prev_ == -1);
}

std::vector<KV> BiList::Pop() {
  std::vector<KV> res;
  while (IsFull()) {
    KV pop_kv = Delete(tail_);
    res.push_back(pop_kv);
  }
  return res;
}

bool BiList::Get(const Slice key, Slice& value) {
  int p = data_[head_].next_;
  while (p != -1) {
    if (data_[p].kv_.key_.compare(key) == 0) {
      value = Slice(data_[p].kv_.value_.data(), data_[p].kv_.value_.size());
      KV pop = Delete(p);
      Insert(pop);
      return true;
    }
    p = data_[p].next_;
  }
  return false;  
}

void BiList::Init(size_t size, size_t numb) {
  head_ = 0;
  tail_ = head_;
  data_size_ = 0;
  data_numb_ = 0;
  max_size_ = size;
  max_numb_ = numb;
  data_.clear();
  data_.resize(max_numb_);
  data_[0].next_ = -1;
  data_[0].prev_ = -1;
  while (!free_.empty()) free_.pop();
  for (size_t i = 1; i <= max_numb_; ++ i)
    free_.push(i);
}

int BiList::ExistAndUpdate(const KV kv) {
  int p = data_[head_].next_;
  while (p != -1) {
    if (data_[p].kv_.key_.compare(kv.key_) == 0) {
      data_[p].kv_ = kv;
      return p;
    }
    p = data_[p].next_;
  }
  return -1;
}

KV BiList::Delete(int pos) {
  assert(pos > 0 && pos <= max_numb_);
  KV kv = data_[pos].kv_;
  if (pos == tail_) 
    tail_ = data_[pos].prev_;
  data_[data_[pos].prev_].next_ = data_[pos].next_;
  if (pos != tail_)
    data_[data_[pos].next_].prev_ = data_[pos].prev_;
  data_[pos].next_ = -1;
  data_[pos].prev_ = -1;
  free_.push(pos);
  data_size_ = data_size_ - kv.size();
  data_numb_ = data_numb_ - 1;
  assert(data_[head_].prev_ == -1);
  return kv;
}

}