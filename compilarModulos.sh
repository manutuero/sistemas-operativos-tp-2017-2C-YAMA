!#/bin/bash

git clone https://github.com/sisoputnfrba/so-commons-library.git
cd  so-commons-library
make install
cd ..

#instalacion utils
cd utils/
make clean
make 
make install
cd ..

#instalacion readline
apt-get install libreadline6-dev

#instalacion master
cd master/
make clean
make
cd ..

#instalacion fileSystem
cd fileSystem
make clean
make 
cd ..

#instalacion Yama
cd yama/
make clean
make
cd ..


#Instalacion de nodo
cd nodo/
rm -rf DataNode
rm -rf Worker

cd dataNode/
make clean
make all
make install
cp DataNode ../
rm DataNode

cd ..
cd worker/
make clean
make all
cp Worker ../
rm Worker

cd ..
cd ..

cp -a scripts/. /home/utnso/thePonchos




