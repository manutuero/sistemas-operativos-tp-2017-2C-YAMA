#!/bin/bash

# Debemos pasarle la ruta al archivo al script para que funcione.
ARCHIVO=$1
if test -z $ARCHIVO
then
	echo 'Debe ingresar la ruta de un achivo.'
	exit 1
fi

grep 'TIPO' $ARCHIVO > aux
grep 'TAMANIO' $ARCHIVO >> aux
grep -E BLOQUE $ARCHIVO | sort -V >> aux

cat aux > $ARCHIVO
rm aux
