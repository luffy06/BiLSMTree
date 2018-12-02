#!/bin/bash
datafolder="data"
resultfolder="result"
suffix=".in"
algos=('LevelDB' 'LevelDB-KV' 'BiLSMTree')
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
  for file in ${datafolder}/*${suffix}; do
    filename=`basename $file`
    echo 'DATA '${filename}
    echo 'RUNNING '${filename} >> ${resultname}
    echo `build/main < $datafolder/$filename` >> ${resultname}
  done
done
echo 'LOADING RESULT'
# echo `python3 loadresult.py`