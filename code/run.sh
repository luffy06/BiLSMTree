#!/bin/bash

set -e  # fail and exit on any command erroring

datafolder="data"
resultfolder="result"
suffix=".in"
algos=('LevelDB-Sep' 'LevelDB' 'Wisckey')
tracepath="trace"
if [[ ! -d ${tracepath} ]]; then
  mkdir ${tracepath}
fi
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
    mv trace.in ${tracepath}/${algo}_${filename}
    date
  done
done
echo 'RUN SUCCESS'
