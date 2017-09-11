/*
 ============================================================================
 Name        : master.c
 Author      : Gaboxxz
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "funcionesMaster.h"

int main(int argsc,char **args) {
	char * transformador;
	char *reductor;
	char *archivoAprocesar;
	char *direccionDeResultado;
	if(argsc==5){
		transformador=args[1];
		reductor=args[2];
		archivoAprocesar=args[3];
		direccionDeResultado=args[4];

		if(!chequearParametros(transformador,reductor,archivoAprocesar,direccionDeResultado)){

			return 0;
		}

	} else {
		printf("Cantidad de parametros invalida. Se esperan 4 pero se detectaron: %d \n",argsc-1);

	}

	return EXIT_SUCCESS;
}
