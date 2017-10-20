#include "funcionesYAMA.h"

int main(void) {

	cargarArchivoDeConfiguracion();
	crearTablaDeEstados();
	//conectarseAFS();
	yamaEscuchando();

	pthread_t hiloConexiones;
	pthread_create(&hiloConexiones, NULL, (void*)escucharMasters, NULL);

	//escucharMasters();
	printf("loop\n");
	while(1){}

	pthread_join(hiloConexiones, NULL);

	return EXIT_SUCCESS;

}



