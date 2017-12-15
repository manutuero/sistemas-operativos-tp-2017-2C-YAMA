#include "funcionesYAMA.h"

int main(void) {

	signal(SIGUSR1,encargadoInterrupciones);
	signal(SIGINT,encargadoInterrupciones);
	cargarArchivoDeConfiguracion();
	crearTablaDeEstados();
	conectarseAFS();
	yamaEscuchando();

	pthread_t hiloConexiones,hiloConeccionFs;
	pthread_create(&hiloConexiones, NULL, (void*)escucharMasters, NULL);
	pthread_create(&hiloConeccionFs,NULL,(void*)escuchaActualizacionesNodos,NULL);

	pthread_join(hiloConexiones, NULL);
	pthread_join(hiloConeccionFs,NULL);

	return EXIT_SUCCESS;

}



