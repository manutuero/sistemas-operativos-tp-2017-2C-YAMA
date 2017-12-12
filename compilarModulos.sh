!#/bin/bash

git clone https://github.com/sisoputnfrba/so-commons-library.git
cd  so-commons-library
sudo make install
cd ..

# Exportamos la variable de entorno
export LC_ALL=C

#instalacion utils
cd utils/
make clean
make 
sudo make install
cd ..

#instalacion readline
sudo apt-get install libreadline6-dev

#instalacion master
cd master/
make clean
make
mkdir logs
chmod 0777 logs
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
cd ..
cd /home/utnso/thePonchos
mkdir tmp
cd tmp
mkdir scripts



