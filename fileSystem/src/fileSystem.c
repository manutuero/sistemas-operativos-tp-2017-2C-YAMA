#include "funcionesFileSystem.h"

sem_t semIpYamaNodos;
int main(int argc, char **argv) {
	pthread_t hiloDatanodes, hiloConsola, hiloYama, hiloWorkers,
			hiloSocketYamaNodos;

	crearLoggerFs();
	// Carga el archivo de configuracion del filesystem.
	ARCHCONFIG = "fsConfig.cfg";
	cargarArchivoDeConfiguracionFS(ARCHCONFIG);
	yamaConectado = 0;

	// Opcion para correr tests.
	if (argc > 1 && sonIguales(argv[1], "--tests")) {
		return correrTests();
	}

	// Inicializacion de semaforos y listas.
	sem_init(&semNodosRequeridos, 0, 0);
	sem_init(&semEstadoEstable, 0, 0);
	sem_init(&semIpYamaNodos, 0, 0);
	nodos = list_create();
	nodosEsperados = list_create();

	// Validacion de estado y creacion de hilos principales.
	pthread_create(&hiloDatanodes, NULL, esperarConexionesDatanodes, NULL);
	if (!hayEstadoAnterior() || (argc > 1 && sonIguales(argv[1], "--clean"))) {
		log_info(fsLogger, "No hay estado anterior, el sistema se ejecuta por primera vez.");
		puts("No hay estado anterior..");
		estadoNodos = ACEPTANDO_NODOS_NUEVOS;
		estadoAnterior = false;

		// Si se recibe un argumento --clean, ignora y elimina el estado anterior.
		if (argc > 1 && sonIguales(argv[1], "--clean"))
			borrarDirectorioMetadata();

		sem_wait(&semNodosRequeridos);
		pthread_create(&hiloConsola, NULL, levantarConsola, NULL);

		log_info(fsLogger,
				"Esperando los nodos requeridos para pasar a un estado estable...");
		sem_wait(&semEstadoEstable);
		pthread_create(&hiloYama, NULL, escucharPeticionesYama, NULL);
		pthread_create(&hiloWorkers, NULL, esperarConexionesWorker, NULL);
		pthread_create(&hiloSocketYamaNodos, NULL, obtenerSocketNodosYama,
		NULL);
	} else if (hayEstadoAnterior()) {
		log_info(fsLogger, "Hay estado anterior, restaurando el sistema...");
		puts("Hay estado anterior..");
		estadoAnterior = true;
		restaurarEstructurasAdministrativas();

		sem_wait(&semEstadoEstable);
		log_info(fsLogger,
				"El filesystem paso a estar estable restaurandose de un estado anterior (hay al menos 1 copia de cada archivo).");
		pthread_create(&hiloYama, NULL, escucharPeticionesYama, NULL);
		pthread_create(&hiloWorkers, NULL, esperarConexionesWorker, NULL);
		pthread_create(&hiloSocketYamaNodos, NULL, obtenerSocketNodosYama,
		NULL);
		sem_wait(&semNodosRequeridos);
		pthread_create(&hiloConsola, NULL, levantarConsola, NULL);
	}

	// Se espera la finalizacion de los hilos.
	pthread_join(hiloDatanodes, NULL);
	pthread_join(hiloConsola, NULL);

	// Se liberan recursos.
	sem_destroy(&semNodosRequeridos);
	sem_destroy(&semEstadoEstable);
	sem_destroy(&semIpYamaNodos);
	free(fsLogger);

	return 0;
}
