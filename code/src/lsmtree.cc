namespace bilsmtree {

LSMTree::LSMTree() {

}

LSMTree::~LSMTree() {

}

Slice LSMTree::Get(const Slice& key) {

}

void LSMTree::AddTableToL0(const Table* table) {

}

size_t LSMTree::GetSequenceNumber() {
  return sequence_number_ ++;
}

void RollBack(size_t file_number) {

}

}