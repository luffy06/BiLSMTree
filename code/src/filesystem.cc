#include "filesystem.h"

FileSystem::FileSystem() {
  fat = new int[MAX_BLOCK_NUMS];
  bool *vis = new bool[MAX_BLOCK_NUMS];
  for (int i = 0; i < MAX_BLOCK_NUMS; ++ i) {
    fat[i] = i;
    vis[i] = false;
  }

  if (Util::ExistFile(FILE_META_PATH)) {
    std::fstream f(FILE_META_PATH, std::ios::in);
    int n;
    f.read((char *)&n, sizeof(int));
    for (int i = 0; i < n; ++ i) {
      int from, to;
      f.read((char *)&from, sizeof(int));
      f.read((char *)&to, sizeof(int));
      fat[from] = to;
      vis[from] = true;
    }

    f.read((char *)&n, sizeof(int));
    for (int i = 0; i < n; ++ i) {
      int size, block_start, filesize;
      char *filename;
      f.read((char *)&size, sizeof(int));
      filename = new char[size + 1];
      f.read(filename, sizeof(char) * size);
      f.read((char *)&block_start, sizeof(int));
      f.read((char *)&filesize, sizeof(int));
      filename[size] = '\0';
      fcbs.push_back(FCB(filename, block_start, filesize));
    }
    f.close();

  }
  for (int i = 0; i < MAX_BLOCK_NUMS; ++ i) {
    if (!vis[i])
      free_blocks.push(i);
  }

  delete[] vis;
  flash = new Flash();
  open_number = 0;
}

FileSystem::~FileSystem() {
  if (Config::PERSISTENCE) {
    std::fstream f(FILE_META_PATH, std::ios::ate | std::ios::out);
    int n = 0;
    for (int i = 0; i < MAX_BLOCK_NUMS; ++ i) {
      if (fat[i] != i)
        ++ n;
    }
    f.write((char *)&n, sizeof(int));
    for (int i = 0; i < n; ++ i) {
      if (fat[i] == i) continue;
      f.write((char *)&i, sizeof(int));
      f.write((char *)&fat[i], sizeof(int));
    }

    n = fcbs.size();
    f.write((char *)&n, sizeof(int));
    for (int i = 0; i < n; ++ i) {
      int size = fcbs[i].filename.size();
      int block_start = fcbs[i].block_start;
      int filesize = fcbs[i].filesize;
      f.write((char *)&size, sizeof(int));
      f.write(fcbs[i].filename.c_str(), sizeof(char) * size);
      f.write((char *)&block_start, sizeof(int));
      f.write((char *)&filesize, sizeof(int));
    }
    f.close();
  }

  delete[] fat;
  delete flash;
}

int FileSystem::Open(const std::string &filename, const int &mode) {
  if (open_number >= MAX_FILE_OPEN) {
    std::cout << "FILE OPEN MAX" << std::endl;
    exit(-1);
  }

  int file_number = -1;
  for (int i = 0; i < fcbs.size(); ++ i) {
    if (filename == fcbs[i].filename) {
      file_number = i;
    }
  }

  if (file_number == -1) {
    FCB fcb = FCB(filename, AssignFreeBlocks(), 0);
    fcbs.push_back(fcb);
    file_number = fcbs.size() - 1;
  }

  int index = BinarySearchInBuffer(file_number);
  if (index != -1) {
    std::cout << "FILE \'" << filename << "\' IS OPEN" << std::endl;
    exit(-1); 
  }

  FileStatus fs;
  fs.file_number = file_number;
  fs.lba = fcbs[file_number].block_start;
  fs.mode = mode;

  file_buffer.push_back(fs);
  open_number ++;
  std::sort(file_buffer.begin(), file_buffer.end(), CmpByFileNumber);
  return file_number;
}

void FileSystem::Read(const int &file_number, std::string &data, const int &read_size) {
  int size = read_size;
  int index = BinarySearchInBuffer(file_number);
  if (index == -1) {
    std::cout << "FILE IS NOT OPEN" << std::endl;
    exit(-1);
  }

  FileStatus &fs = file_buffer[index];
  if (!(fs.mode & Config::READ_OPTION)) {
    std::cout << "CANNOT READ WITHOUT READ PERMISSION" << std::endl;
    exit(-1);
  }

  data = "";
  while (size > 0 && fat[fs.lba] != fs.lba) {
    char *c_data = flash->Read(fs.lba);
    c_data[std::min(size, BLOCK_SIZE)] = '\0';
    data = data + std::string(c_data);
    size = size - BLOCK_SIZE;
    fs.lba = fat[fs.lba];
  }
}

void FileSystem::Write(const int &file_number, const std::string &data, const int &write_size) {
  int index = BinarySearchInBuffer(file_number);
  if (index == -1) {
    std::cout << "FILE IS NOT OPEN" << std::endl;
    exit(-1);
  }

  FileStatus &fs = file_buffer[index];
  if (!(fs.mode & (Config::WRITE_OPTION | Config::APPEND_OPTION))) {
    std::cout << "CANNOT WRITE WITHOUT WRITE PERMISSION" << std::endl;
    exit(-1);
  }

  if (data.size() == 0)
    return ;

  int need_blocks = data.size() % BLOCK_SIZE == 0 ? data.size() / BLOCK_SIZE : data.size() / BLOCK_SIZE + 1;
  // std::cout << "NEED BLOCKS:" << need_blocks << std::endl;
  if (fs.mode & Config::APPEND_OPTION) {
    while (fs.lba != fat[fs.lba]) fs.lba = fat[fs.lba];

    for (int i = 0; i < need_blocks; ++ i) {
      int l = i * BLOCK_SIZE;
      int r = std::min((int)data.size(), (i + 1) * BLOCK_SIZE);
      flash->Write(fs.lba, data.substr(l, r).c_str()); // TODO: LOW EFFCIENCY
      int new_block = AssignFreeBlocks();
      fat[fs.lba] = new_block;
      fs.lba = fat[fs.lba];
    }
  }
  else {
    fs.lba = fcbs[fs.file_number].block_start;
    for (int i = 0; need_blocks > 0 && fs.lba != fat[fs.lba]; ++ i, -- need_blocks) {
      int l = i * BLOCK_SIZE;
      int r = std::min((int)data.size(), (i + 1) * BLOCK_SIZE);
      flash->Write(fs.lba, data.substr(l, r).c_str()); // TODO: LOW EFFCIENCY
      fs.lba = fat[fs.lba];
    }
    if (need_blocks > 0) {
      for (int i = 0; i < need_blocks; ++ i) {
        int l = i * BLOCK_SIZE;
        int r = std::min((int)data.size(), (i + 1) * BLOCK_SIZE);
        flash->Write(fs.lba, data.substr(l, r).c_str()); // TODO: LOW EFFCIENCY
        int new_block = AssignFreeBlocks();
        fat[fs.lba] = new_block;
        fs.lba = fat[fs.lba];
      }
    }
    else if (fs.lba != fat[fs.lba]) {
      int lba = fs.lba;
      while (lba != fat[lba]) {
        int next_lba = fat[lba];
        FreeBlock(lba);
        lba = next_lba;
      }
    }
  }
}

void FileSystem::Close(const int &file_number) {
  int index = BinarySearchInBuffer(file_number);
  if (index == -1)
    return ;

  open_number --;
  file_buffer.erase(file_buffer.begin() + index); // TODO: LOW EFFCIENCY
}

int FileSystem::BinarySearchInBuffer(const int &file_number) {
  if (file_buffer.size() == 0)
    return -1;
  int l = 0;
  int r = file_buffer.size() - 1;
  if (file_number < file_buffer[l].file_number)
    return -1;
  else if (file_number > file_buffer[r].file_number)
    return -1;
  else if (file_number == file_buffer[r].file_number)
    return r;
  while (r - l > 1) {
    int m = (l + r) / 2;
    if (file_buffer[m].file_number <= file_number)
      l = m;
    else
      r = m;
  }
  if (file_buffer[l].file_number == file_number)
    return l;
  return -1;
}


bool FileSystem::CmpByFileNumber(const FileStatus &fs1, const FileStatus &fs2) {
  return fs1.file_number < fs2.file_number;
}


int FileSystem::AssignFreeBlocks() {
  if (free_blocks.size() == 0) {
    std::cout << "NOT ENOUGH FREE BLOCKS" << std::endl;
    exit(-1);
  }
  int new_block = free_blocks.front();
  free_blocks.pop();
  return new_block;
}

void FileSystem::FreeBlock(const int &lba) {
  fat[lba] = lba;
  free_blocks.push(lba);
}


