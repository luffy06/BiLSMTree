#!/bin/bash

set -e  # fail and exit on any command erroring

testid=0
datafolder="data"
resultfolder="result"
suffix=".in"
algos=('LevelDB' 'BiLSMTree' 'Wisckey')
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
done
echo 'LOADING RESULT'
echo `python3 loadresult.py`