#!/bin/bash
buildpath="build"
datapath="data"
logpath="logs"
blockpath="blocks"
tracepath="trace"
libpath="lib"
if [[ -d ${logpath} ]]; then
  rm -rf ${logpath}
fi
mkdir ${logpath}
mkdir ${logpath}/${blockpath}
if [[ ! -d ${datapath} ]]; then
  mkdir ${datapath}
fi
if [[ -d ${tracepath} ]]; then
  rm -rf ${tracepath}
fi
mkdir ${tracepath}
if [[ -d ${buildpath} ]]; then
  rm -rf ${buildpath}
fi
mkdir ${buildpath}
cd ${buildpath}
cmake .. && cmake --build .
cd ..
if [[ ! -d ${libpath} ]]; then
  mkdir ${libpath}
fi
cd ${libpath}
if [[ ! -d YCSB ]]; then
  git clone https://github.com/brianfrankcooper/YCSB
fi
cd YCSB
bin/ycsb.sh load basic -P workloads/workloada
