rm -rf lib/
mkdir lib
cd lib/
# git clone https://github.com/google/leveldb
# cd leveldb/
# mkdir -p build && cd build
# cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
cd ../../
wget http://prdownloads.sourceforge.net/threadpool/threadpool-0_2_5-src.zip
tar -zxvf threadpool-0_2_5-src.zip
mv threadpool-0_2_5-src/threadpool/boost threadpool
rm -rf threadpool-0_2_5-src threadpool-0_2_5-src.zip
git clone https://github.com/HdrHistogram/HdrHistogram_c
git clone https://github.com/brianfrankcooper/YCSB