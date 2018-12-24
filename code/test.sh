#!/bin/bash

set -e  # fail and exit on any command erroring

testid=0
datafolder="data"
resultfolder="result"
suffix=".in"
algos=('LevelDB-Sep' 'LevelDB' 'Wisckey')
tracepath="trace"
for algo in ${algos[*]}; do
  if [[ -f 'config.in' ]]; then
    rm 'config.in'
  fi
  echo ${algo} >> config.in
  resultname=${resultfolder}/${algo}.out
  if [[ -f ${resultname} ]]; then
    rm ${resultname}
  fi
  echo 'Running '${algo} 
  filename=`basename data$testid.in`
  echo 'RUNNING '${filename}
  echo 'RUNNING '${filename} >> ${resultname}
  echo `build/main < $datafolder/$filename` >> ${resultname}
  mv trace.in ${tracepath}/${algo}_${filename}
done
echo 'RUN SUCCESS'
