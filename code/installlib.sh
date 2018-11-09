#!/bin/bash
libpath="lib"
if [[ ! -d ${libpath} ]]; then
  mkdir ${libpath}
fi
cd ${libpath}
# Download leveldb and build
# if [[ ! -d leveldb ]]; then
#   git clone https://github.com/google/leveldb
#   cd leveldb/
#   mkdir -p build && cd build
#   cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
#   cd ../../
# fi
# Download threadpool
if [[ ! -d threadpool ]]; then
  wget http://prdownloads.sourceforge.net/threadpool/threadpool-0_2_5-src.zip
  tar -zxvf threadpool-0_2_5-src.zip
  mv threadpool-0_2_5-src/threadpool/boost threadpool
  rm -rf threadpool-0_2_5-src threadpool-0_2_5-src.zip
fi
# Download HdrHistogram
# if [[ ! -d HdrHistogram_c ]]; then
#   git clone https://github.com/HdrHistogram/HdrHistogram_c
# fi
# Download YCSB
if [[ ! -d YCSB ]]; then
  git clone https://github.com/brianfrankcooper/YCSB
  cd YCSB
  bin/ycsb.sh load basic -P workloads/workloada
fi
