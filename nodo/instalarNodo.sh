!#/bin/bash

rm DataNode
rm Worker

cd dataNode/
make all
make install
cp DataNode ../
rm DataNode

cd ..
cd worker/
make all
cp Worker ../
rm Worker




