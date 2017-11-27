#include "funcionesFileSystem.h"

sem_t semIpYamaNodos;
int main(int argc, char **argv) {
	pthread_t hiloDatanodes, hiloConsola, hiloYama, hiloWorkers,
			hiloSocketYamaNodos;

	ARCHCONFIG = "fsConfig.cfg";
	cargarArchivoDeConfiguracionFS(ARCHCONFIG);
	yamaConectado=0;
	// Opcion para correr tests.
	if (argc > 1 && sonIguales(argv[1], "--tests")) {
		return correrTests();
	}

	sem_init(&semNodosRequeridos, 0, 0);
	sem_init(&semEstadoEstable, 0, 0);
	sem_init(&semIpYamaNodos, 0, 0);

	nodos = list_create();
	nodosEsperados = list_create();

	// Agregar un logger (cuando estemos por terminar).
	pthread_create(&hiloDatanodes, NULL, esperarConexionesDatanodes, NULL);

	if (!hayEstadoAnterior() || (argc > 1 && sonIguales(argv[1], "--clean"))) {
		puts("No hay estado anterior..");
		estadoNodos = ACEPTANDO_NODOS_NUEVOS;
		estadoAnterior = false;

		if (argc > 1 && sonIguales(argv[1], "--clean"))
			borrarDirectorioMetadata();

		sem_wait(&semNodosRequeridos);
		pthread_create(&hiloConsola, NULL, levantarConsola, NULL);

		sem_wait(&semEstadoEstable);
		pthread_create(&hiloYama, NULL, escucharPeticionesYama, NULL);
		pthread_create(&hiloWorkers, NULL, esperarConexionesWorker, NULL);
		pthread_create(&hiloSocketYamaNodos, NULL, obtenerSocketNodosYama, NULL);
	} else if (hayEstadoAnterior()) {
		puts("Hay estado anterior..");
		estadoAnterior = true;
		restaurarEstructurasAdministrativas();

		sem_wait(&semEstadoEstable);
		puts("Paso a estable! :)");
		pthread_create(&hiloYama, NULL, escucharPeticionesYama, NULL);
		pthread_create(&hiloWorkers, NULL, esperarConexionesWorker, NULL);
		pthread_create(&hiloSocketYamaNodos, NULL, obtenerSocketNodosYama, NULL);
		sem_wait(&semNodosRequeridos);
		pthread_create(&hiloConsola, NULL, levantarConsola, NULL);
	}

	pthread_join(hiloDatanodes, NULL);
	pthread_join(hiloConsola, NULL);

	// Libero recursos.
	sem_destroy(&semNodosRequeridos);
	sem_destroy(&semEstadoEstable);
	sem_destroy(&semIpYamaNodos);
	return 0;
}
