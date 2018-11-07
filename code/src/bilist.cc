#include "bilist.h"

namespace bilsmtree {

BiList::BiList(size_t size) {
  head_ = 0;
  tail_ = 0;
  data_size_ = 0;
  max_size_ = size;
  data_ = new ListNode[size + 1];
  for (size_t i = 1; i <= size; ++ i)
    free_.push(i);
}

BiList::~BiList() {
  head_ = 0;
  tail_ = 0;
  delete data_;
  free_.clear();
}

void BiList::Set(size_t pos, KV kv) {
  Util::Assert("Position is exceeded!\nMethod: BiList::Set", (pos > 0 && pos <= max_size_));
  data_[pos].kv_ = kv;
}

void BiList::MoveToHead(size_t pos) {
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

KV BiList::Get(size_t pos) {
  Util::Assert("Position is exceeded!\nMethod: BiList::Get", (pos > 0 && pos <= max_size_));
  return data_[pos].kv_;
}

KV BiList::Delete(size_t pos) {
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

KV BiList::Append(KV kv) {
  KV res = NULL;
  if (free_.empty())
    res = Delete(tail_);
  size_t pos = free_.front();
  free_.pop();
  data_size_ = data_size_ + 1;
  data_[pos].kv_ = kv;
  data_[pos].prev_ = tail_;
  data_[pos].next_ = 0;
  data_[tail_].next_ = pos;
  tail_ = pos;
  return res;
}

KV Insert(KV kv) {
  KV res = NULL;
  if (free_.empty())
    res = Delete(data_[head_].next_);
  size_t pos = free_.front();
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