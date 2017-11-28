!#/bin/bash


#instalacion utils
cd utils/
make clean
cd ..

#instalacion master
cd master/
make clean
cd ..

#instalacion fileSystem
cd fileSystem
make clean
cd ..

#instalacion Yama
cd yama/
make clean
cd ..


#Instalacion de nodo
cd nodo/
rm -rf DataNode
rm -rf Worker

cd dataNode/
make clean
cd ..
cd worker/
make clean




