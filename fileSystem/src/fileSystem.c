#include "funcionesFileSystem.h"

int main(int argc, char **argv) {
	pthread_t hiloConexiones, hiloConsola, hiloYama;

	ARCHCONFIG = "fsConfig.cfg";
	cargarArchivoDeConfiguracionFS(ARCHCONFIG);

	// Opcion para correr tests.
	if (argc > 1 && sonIguales(argv[1], "--tests")) {
		return correrTests();
	}

	sem_init(&semNodosRequeridos, 0, 0);
	sem_init(&semEstadoEstable, 0, 0);
	nodos = list_create();

	// Agregar un logger (cuando estemos por terminar).

	pthread_create(&hiloConexiones, NULL, esperarConexionesDatanodes, NULL);
	if (hayEstadoAnterior()) {
		//archivos = list_create();
		puts("Hay estado anterior..");
		restaurarEstructurasAdministrativas();
	} else {
		puts("No hay estado anterior. Esperando datanodes...");
		estadoNodos=ACEPTANDO_NODOS_NUEVOS;
		sem_wait(&semNodosRequeridos);
		pthread_create(&hiloConsola, NULL, levantarConsola, NULL);
		sem_destroy(&semNodosRequeridos);

		sem_wait(&semEstadoEstable);
		pthread_create(&hiloYama, NULL, escucharPeticionesYama, NULL);
	}

	/* Tambien agregar conexion con worker para recibir el archivo resultante a la reduccion global. */

	pthread_join(hiloConexiones, NULL);
	pthread_join(hiloConsola, NULL);

	// Libero recursos.
	sem_destroy(&semNodosRequeridos);
	return 0;
}
