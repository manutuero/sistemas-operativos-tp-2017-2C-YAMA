/*
 ============================================================================
 Name        : yama.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "funcionesYAMA.h"

int main(void) {

	cargarArchivoDeConfiguracion();
	conectarseAFS();
	yamaEscuchando();
	escucharMasters();
	while(1){}

	return EXIT_SUCCESS;

}



 