/*
 ============================================================================
 Name        : yama.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "funcionesYAMA.h"

int main(void) {

	cargarArchivoDeConfiguracion();
	crearTablaDeEstados();
	conectarseAFS();
	yamaEscuchando();

	pthread_t hiloConexiones,hiloConeccionFs;
	pthread_create(&hiloConexiones, NULL, (void*)escucharMasters, NULL);
	pthread_create(&hiloConeccionFs,NULL,(void*)escuchaActualizacionesNodos,NULL);
	//escucharMasters();
	printf("loop\n");
	while(1){}

	pthread_join(hiloConexiones, NULL);
	pthread_join(hiloConeccionFs,NULL);


	return EXIT_SUCCESS;

}



