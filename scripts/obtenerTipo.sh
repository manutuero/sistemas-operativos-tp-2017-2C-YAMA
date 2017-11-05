#!/bin/bash
#El pipe espera como primer argumento un archivo (argv[1]).

TIPO=$(file -i  "$1" | grep -oP 'text/plain')
if test "${TIPO}" = 'text/plain' 
then 
	echo "1" # Es un archivo de texto plano.
fi

TIPO=$(file -i  "$1" | grep -oP 'text/x-shellscript')
if test "${TIPO}" = 'text/x-shellscript'
then 
	echo "1" # Es un archivo de texto plano (script).
fi

TIPO=$(file -i  "$1" | grep -oP 'binary')
if test "${TIPO}" = 'binary'
then 
	echo "0" # Es un archivo binario.
fi
