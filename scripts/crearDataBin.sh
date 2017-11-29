#!/bin/bash
# Crea un data.bin de 'BLOQUES' en el directorio actual del script.
BLOQUES=$1
UN_MEGABYTE=1048576
TAMANIO=$(expr $BLOQUES \* $UN_MEGABYTE)

touch data.bin
truncate -s $TAMANIO data.bin
