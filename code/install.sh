#!/bin/bash
datapath="data"
logpath="logs"
blockpath="blocks"
libpath="lib"
if [[ -d ${logpath} ]]; then
  rm -rf ${logpath}
fi
mkdir ${logpath}
mkdir ${logpath}/${blockpath}
if [[ ! -d ${datapath} ]]; then
  mkdir ${datapath}
fi
if [[ ! -d ${libpath} ]]; then
  mkdir ${libpath}
fi
cd ${libpath}
if [[ ! -d YCSB ]]; then
  git clone https://github.com/brianfrankcooper/YCSB
fi
cd YCSB
bin/ycsb.sh load basic -P workloads/workloada
