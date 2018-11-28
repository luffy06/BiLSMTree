#include "virtualfilesystem.h"

namespace bilsmtree {

VirtualFileSystem::VirtualFileSystem() {
  
}

VirtualFileSystem::~VirtualFileSystem() {

}

void VirtualFileSystem::Create(const std::string filename) {

}

size_t VirtualFileSystem::Open(const std::string filename, const size_t mode) {
  return 0;
}

void VirtualFileSystem::Seek(const size_t file_number, const size_t offset) {

}

void VirtualFileSystem::SetFileSize(const size_t file_number, const size_t file_size) {

}

std::string VirtualFileSystem::Read(const size_t file_number, const size_t read_size) {
  return std::string("");
}

void VirtualFileSystem::Write(const size_t file_number, const char* data, const size_t write_size) {

}

void VirtualFileSystem::Delete(const std::string filename) {

}

void VirtualFileSystem::Close(const size_t file_number) {

}

}