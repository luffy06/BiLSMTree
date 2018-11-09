#include "bilist.h"

namespace bilsmtree {

BiList::BiList(uint size) {
  head_ = 0;
  tail_ = 0;
  data_size_ = 0;
  max_size_ = size;
  data_ = new ListNode[size + 1];
  for (uint i = 1; i <= size; ++ i)
    free_.push(i);
}

BiList::~BiList() {
  head_ = 0;
  tail_ = 0;
  delete data_;
}

void BiList::Set(uint pos, const KV kv) {
  Util::Assert("Position is exceeded!\nMethod: BiList::Set", (pos > 0 && pos <= max_size_));
  data_[pos].kv_ = kv;
}

void BiList::MoveToHead(uint pos) {
  Util::Assert("Position is exceeded!\nMethod: BiList::MoveToHead", (pos > 0 && pos <= max_size_));
  if (pos == tail_) {
    tail_ = data_[tail_].prev_;
    data_[tail_].next_ = 0;
  }
  data_[pos].next_ = data_[head_].next_;
  data_[pos].prev_ = head_;
  data_[head_].next_ = pos;
  data_[data_[pos].next_].prev_ = pos;
}

KV BiList::Get(uint pos) {
  Util::Assert("Position is exceeded!\nMethod: BiList::Get", (pos > 0 && pos <= max_size_));
  return data_[pos].kv_;
}

KV BiList::Delete(uint pos) {
  Util::Assert("Position is exceeded!\nMethod: BiList::Delete", (pos > 0 && pos <= max_size_));
  KV kv = data_[pos].kv_;
  data_[data_[pos].prev_].next_ = data_[pos].next_;
  if (pos != tail_)
    data_[data_[pos].next_].prev_ = data_[pos].prev_;
  else
    tail_ = data_[pos].prev_;
  free_.push(pos);
  data_size_ = data_size_ - 1;
  return kv;
}

bool BiList::Append(const KV kv, KV& pop_kv) {
  bool res = false;
  if (free_.empty()) {
    pop_kv = Delete(tail_);
    res = true;
  }
  uint pos = free_.front();
  free_.pop();
  data_size_ = data_size_ + 1;
  data_[pos].kv_ = kv;
  data_[pos].prev_ = tail_;
  data_[pos].next_ = 0;
  data_[tail_].next_ = pos;
  tail_ = pos;
  return res;
}

bool BiList::Insert(const KV kv, KV& pop_kv) {
  bool res = false;
  if (free_.empty()) {
    pop_kv = Delete(data_[head_].next_);
    res = true;
  }
  uint pos = free_.front();
  free_.pop();
  data_size_ = data_size_ + 1;
  data_[pos].kv_ = kv;
  data_[pos].prev_ = head_;
  data_[pos].next_ = data_[head_].next_;
  data_[data_[head_].next_].prev_ = pos;
  data_[head_].next_ = pos;
  return res;
}

}