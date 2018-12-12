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

void BiList::Set(size_t pos, const KV kv) {
  if (pos <= 0 || pos > max_numb_)
    std::cout << pos << "\t" << max_numb_ << std::endl;
  assert(pos > 0 && pos <= max_numb_);
  data_[pos].kv_ = kv;
}

void BiList::MoveToHead(size_t pos) {
  assert(pos > 0 && pos <= max_numb_);
  KV kv = Delete(pos);
  KV pop_kv;
  assert(!Insert(kv, pop_kv));
  // std::cout << "Move " << pos << std::endl;
  // std::cout << "Tail " << tail_ << std::endl;  
  // if (pos == tail_) {
  //   tail_ = data_[tail_].prev_;
  //   data_[tail_].next_ = 0;
  // }
  // if (data_[pos])
  // data_[pos].prev_ = head_;
  // data_[pos].next_ = data_[head_].next_;
  // if (data_[head_].next_ != 0)
  //   data_[data_[head_].next_].prev_ = pos;
  // data_[head_].next_ = pos;
  assert(data_[head_].prev_ == 0);
}

KV BiList::Get(size_t pos) {
  assert(pos > 0 && pos <= max_numb_);
  return data_[pos].kv_;
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

bool BiList::Append(const KV kv, KV& pop_kv) {
  bool res = false;
  if (IsFull()) {
    pop_kv = Delete(data_[head_].next_);
    res = true;
  }
  size_t pos = free_.front();
  free_.pop();
  data_size_ = data_size_ + kv.size();
  data_[pos].kv_ = kv;
  data_[pos].prev_ = tail_;
  data_[pos].next_ = 0;
  data_[tail_].next_ = pos;
  tail_ = pos;
  assert(data_[head_].prev_ == 0);
  return res;
}

bool BiList::Insert(const KV kv, KV& pop_kv) {
  bool res = false;
  if (IsFull()) {
    pop_kv = Delete(tail_);
    res = true;
  }
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
  assert(data_[head_].prev_ == 0);
  return res;
}

}