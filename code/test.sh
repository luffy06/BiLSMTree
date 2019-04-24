#!/bin/bash

set -e  # fail and exit on any command erroring

testid=(1 10 8 12)
datafolder="data"
resultfolder="result"
suffix=".in"
algos=('BiLSMTree' 'Wisckey')
for algo in ${algos[*]}; do
  resultname=${resultfolder}/${algo}.out
  if [[ -f ${resultname} ]]; then
    rm ${resultname}
  fi
done
for id in ${testid[*]}; do
  for algo in ${algos[*]}; do
    if [[ -f 'config.in' ]]; then
      rm 'config.in'
    fi
    echo ${algo} >> config.in
    resultname=${resultfolder}/${algo}.out
    echo 'Running '${algo} 
    filename=`basename data$id.in`
    date
    echo 'RUNNING '${filename}
    echo 'RUNNING '${filename} >> ${resultname}
    echo `build/main < $datafolder/$filename` >> ${resultname}
    date
  done
done
echo 'RUN SUCCESS'
