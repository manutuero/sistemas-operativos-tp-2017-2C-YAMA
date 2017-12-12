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

	signal(SIGINT,encargadoInterrupciones);
	cargarArchivoDeConfiguracion();
	crearTablaDeEstados();
	conectarseAFS();
	yamaEscuchando();

	pthread_t hiloConexiones,hiloConeccionFs;
	pthread_create(&hiloConexiones, NULL, (void*)escucharMasters, NULL);
	pthread_create(&hiloConeccionFs,NULL,(void*)escuchaActualizacionesNodos,NULL);
	//escucharMasters();

	pthread_join(hiloConexiones, NULL);
	pthread_join(hiloConeccionFs,NULL);


	return EXIT_SUCCESS;

}



