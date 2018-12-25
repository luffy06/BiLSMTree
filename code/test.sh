#!/bin/bash

set -e  # fail and exit on any command erroring

testid=0
datafolder="data"
resultfolder="result"
suffix=".in"
algos=('LevelDB-Sep' 'Wisckey' 'LevelDB')
tracepath="trace"
if [[ ! -d ${tracepath} ]]; then
  mkdir ${tracepath}
fi
for algo in ${algos[*]}; do
  if [[ -f 'config.in' ]]; then
    rm 'config.in'
  fi
  echo ${algo} >> config.in
  resultname=${resultfolder}/${algo}.out
  if [[ -f ${resultname} ]]; then
    rm ${resultname}
  fi
  if [[ -f trace.in ]]; then
    rm trace.in
  fi
  echo 'Running '${algo} 
  filename=`basename data$testid.in`
  date
  echo 'RUNNING '${filename}
  echo 'RUNNING '${filename} >> ${resultname}
  echo `build/main < $datafolder/$filename` >> ${resultname}
  mv trace.in ${tracepath}/${algo}_${filename}
  date
done
echo 'RUN SUCCESS'