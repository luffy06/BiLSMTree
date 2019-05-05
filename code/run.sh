#!/bin/bash

set -e  # fail and exit on any command erroring

datafolder="data"
resultfolder="result"
suffix=".in"
algos=('BiLSMTree' 'Wisckey' 'BiLSMTree-Ext' 'LevelDB')
for algo in ${algos[*]}; do
  resultname=${resultfolder}/${algo}.out
  if [[ -f ${resultname} ]]; then
    rm ${resultname}
  fi
done
for file in ${datafolder}/*${suffix}; do
  for algo in ${algos[*]}; do
    if [[ -f 'config.in' ]]; then
      rm 'config.in'
    fi
    echo ${algo} >> config.in
    resultname=${resultfolder}/${algo}.out
    echo 'Running '${algo} 
    filename=`basename $file`
    date
    echo 'RUNNING '${filename}
    echo 'RUNNING '${filename} >> ${resultname}
    echo `build/main < $datafolder/$filename` >> ${resultname}
    date
  done
done
echo 'LOADING RESULT'
echo `python3 loadresult.py`