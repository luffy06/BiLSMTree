rm -rf leveldb/
rm -rf threadpool/
rm -rf HdrHistogram_c/
git clone https://github.com/google/leveldb
wget http://prdownloads.sourceforge.net/threadpool/threadpool-0_2_5-src.zip
tar -zxvf threadpool-0_2_5-src.zip
mv threadpool-0_2_5-src/threadpool/boost threadpool
rm -rf threadpool-0_2_5-src threadpool-0_2_5-src.zip
git clone https://github.com/HdrHistogram/HdrHistogram_c