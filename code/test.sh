#!/bin/bash

set -e  # fail and exit on any command erroring

testids=(0 17)
datafolder="data"
resultfolder="result"
suffix=".in"
algos=('LevelDB-Sep' 'Wisckey')
tracepath="trace"
temppath="trace.in"
if [[ -d ${tracepath} ]]; then
  rm -rf ${tracepath}
fi
mkdir ${tracepath}
for algo in ${algos[*]}; do
  resultname=${resultfolder}/${algo}.out
  if [[ -f ${resultname} ]]; then
    rm ${resultname}
  fi
done
for testid in ${testids[*]}; do
  for algo in ${algos[*]}; do
    if [[ -f 'config.in' ]]; then
      rm 'config.in'
    fi
    echo ${algo} >> config.in
    resultname=${resultfolder}/${algo}.out
    if [[ -f $temppath ]]; then
      rm $temppath
    fi
    echo 'Running '${algo} 
    filename=`basename data$testid.in`
    date
    echo 'RUNNING '${filename}
    echo 'RUNNING '${filename} >> ${resultname}
    echo `build/main < $datafolder/$filename` >> ${resultname}
    date
    echo `python3 preprocess.py $algo $filename $temppath $tracepath`
  done
done
echo 'RUN SUCCESS'
