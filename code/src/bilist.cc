#include "bilist.h"

namespace bilsmtree {

BiList::BiList(size_t size, size_t numb) {
  head_ = 0;
  tail_ = 0;
  data_size_ = 0;
  data_numb_ = 0;
  max_size_ = size;
  max_numb_ = numb;
  data_.resize(max_numb_);
  data_[0].next_ = 0;
  data_[0].prev_ = 0;
  for (size_t i = 1; i <= max_numb_; ++ i)
    free_.push(i);
}

BiList::~BiList() {
  head_ = 0;
  tail_ = 0;
  data_.clear();
}

bool BiList::IsFull() {
  if (free_.empty())
    return true;
  if (data_size_ >= max_size_)
    return true;
  return false;
}

std::vector<KV> BiList::Append(const KV kv) {
  std::vector<KV> res;
  if (ExistAndUpdate(kv) != -1)
    return res;
  size_t pos = free_.front();
  free_.pop();
  data_size_ = data_size_ + kv.size();
  data_[pos].kv_ = kv;
  data_[pos].prev_ = tail_;
  data_[pos].next_ = 0;
  data_[tail_].next_ = pos;
  tail_ = pos;
  while (IsFull()) {
    KV pop_kv = Delete(data_[head_].next_);
    res.push_back(pop_kv);
  }
  assert(data_[head_].prev_ == 0);
  return res;
}

std::vector<KV> BiList::Insert(const KV kv) {
  std::vector<KV> res;
  if (ExistAndUpdate(kv) != -1)
    return res;
  size_t pos = free_.front();
  free_.pop();
  data_size_ = data_size_ + kv.size();
  data_[pos].kv_ = kv;
  data_[pos].prev_ = head_;
  data_[pos].next_ = data_[head_].next_;
  if (data_[head_].next_ != 0)
    data_[data_[head_].next_].prev_ = pos;
  data_[head_].next_ = pos;
  if (tail_ == 0)
    tail_ = pos;
  while (IsFull()) {
    KV pop_kv = Delete(tail_);
    res.push_back(pop_kv);
  }
  assert(data_[head_].prev_ == 0);
  return res;
}

bool BiList::Get(const Slice key, Slice& value) {
  size_t p = data_[head_].next_;
  while (p) {
    if (data_[p].kv_.key_.compare(key) == 0) {
      value = Slice(data_[p].kv_.value_.data(), data_[p].kv_.value_.size());
      KV pop = Delete(p);
      std::vector<KV> temp = Insert(pop);
      assert(temp.size() == 0);
      return true;
    }
    p = data_[p].next_;
  }
  return false;  
}

int BiList::ExistAndUpdate(const KV kv) {
  size_t p = data_[head_].next_;
  while (p) {
    if (data_[p].kv_.key_.compare(kv.key_) == 0) {
      data_[p].kv_ = kv;
      return p;
    }
    p = data_[p].next_;
  }
  return -1;
}

KV BiList::Delete(size_t pos) {
  assert(pos > 0 && pos <= max_numb_);
  KV kv = data_[pos].kv_;
  if (pos == tail_) {
    tail_ = data_[pos].prev_;
    data_[tail_].next_ = 0;
  }
  data_[data_[pos].prev_].next_ = data_[pos].next_;
  if (data_[pos].next_ != 0)
    data_[data_[pos].next_].prev_ = data_[pos].prev_;
  data_[pos].next_ = 0;
  data_[pos].prev_ = 0;
  free_.push(pos);
  data_size_ = data_size_ - kv.size();
  assert(data_[head_].prev_ == 0);
  return kv;
}

}