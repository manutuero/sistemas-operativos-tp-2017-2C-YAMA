/*
 * funcionesMaster.c
 *
 *  Created on: 11/9/2017
 *      Author: utnso
 */

#include "funcionesMaster.h"
#include <dirent.h>

int chequearParametros(char *transformador, char *reductor,
		char *archivoAprocesar, char *direccionDeResultado) {

	char * comienzo = "yamafs:/";
	if (string_starts_with(archivoAprocesar, comienzo) <= 0) {
		printf("Parametro archivo a procesar invalido.: %s \n",
				archivoAprocesar);
		return 0;
	}
	if (string_starts_with(direccionDeResultado, comienzo) <= 0) {
		printf("La direccion de guardado de resultado es invalida: %s \n",
				direccionDeResultado);
		return 0;
	}

	if (!file_exists(transformador)) {
		printf("El programa transformador no se encuentra en : %s  \n",
				transformador);
		return 0;
	}
	if (!file_exists(reductor)) {
		printf("El programa reductor no se encuentra en : %s  \n", reductor);
		return 0;
	}

	return 1;

}
//Chequea existencia de archivo en linux
int file_exists(char * fileName) {
	struct stat buf;
	int i = stat(fileName, &buf);
	/* File found*/
	if (i == 0) {
		return 1;
	}
	return 0;

}

void crearListas() {
	listaTransformaciones = list_create();
	listaRedLocales = list_create();
	listaRedGloblales = list_create();
	archivosTranformacionOk = list_create();
}

void destruirListas() {
	list_destroy_and_destroy_elements(listaTransformaciones, liberarTransformaciones);
	list_destroy_and_destroy_elements(listaRedLocales, liberarReduccionesLocales);
	list_destroy_and_destroy_elements(listaRedGloblales, liberarReduccionesGlobales);
	list_destroy_and_destroy_elements(archivosTranformacionOk, free);

	pthread_mutex_destroy(&mutexMaximasTareas);
	pthread_mutex_destroy(&mutexTotalTransformaciones);
	pthread_mutex_destroy(&mutexTotalReduccionesLocales);
	pthread_mutex_destroy(&mutexConexionWorker);
	pthread_mutex_destroy(&mutexReplanificado);
}

void liberarTransformaciones(void* transformacion){
	free(((t_transformacionMaster*) transformacion)->ip);
	free(((t_transformacionMaster*) transformacion)->archivoTransformacion);
	free(transformacion);
}

void liberarReduccionesLocales(void* reduccion){
	free(((t_reduccionLocalMaster*) reduccion)->ip);
	free(((t_reduccionLocalMaster*) reduccion)->archivoTransformacion);
	free(((t_reduccionLocalMaster*) reduccion)->archivoRedLocal);
	free(reduccion);
}

void liberarReduccionesGlobales(void* reduccion){
	free(((t_reduccionGlobalMaster*) reduccion)->ip);
	free(((t_reduccionGlobalMaster*) reduccion)->archivoRedGlobal);
	free(((t_reduccionGlobalMaster*) reduccion)->archivoRedLocal);
	free(reduccion);
}

void limpiarListas(){
	list_clean_and_destroy_elements(listaTransformaciones, liberarTransformaciones);
	list_clean_and_destroy_elements(listaRedLocales, liberarReduccionesLocales);
	list_clean_and_destroy_elements(listaRedGloblales, liberarReduccionesGlobales);
}

void inicializarMutex() {
	pthread_mutex_init(&mutexMaximasTareas, NULL);
	pthread_mutex_init(&mutexTotalTransformaciones, NULL);
	pthread_mutex_init(&mutexTotalReduccionesLocales, NULL);
	pthread_mutex_init(&mutexConexionWorker, NULL);
	pthread_mutex_init(&mutexTiempoTransformaciones, NULL);
	pthread_mutex_init(&mutexTiempoReducciones, NULL);
	pthread_mutex_init(&mutexTotalFallos, NULL);
	pthread_mutex_init(&mutexReplanificado, NULL);

}

void crearLogger() {

	char* pathLogger = string_new();
	char* temporal = string_new();
	char* sTime = temporal_get_string_time();
	char cwd[1024];
	string_append(&pathLogger, getcwd(cwd, sizeof(cwd)));
	string_append(&pathLogger, "/logs/");
	DIR* directory = opendir(pathLogger);
	char* comando = string_new();
	if(!directory){
		string_append(&comando, "mkdir ");
		string_append(&comando, pathLogger);
		system(comando);
	}
	else{
		closedir(directory);
	}
	string_append(&temporal, "masterLogs");

	string_append(&temporal, sTime);
	string_append(&temporal,".log");
	string_append(&pathLogger, temporal);

	char *logMasterFileName = strdup(temporal);
	masterLogger = log_create(pathLogger, logMasterFileName, true,
			LOG_LEVEL_TRACE);
	free(sTime);
	free(temporal);
	free(logMasterFileName);
	free(pathLogger);
	free(comando);
	logMasterFileName = NULL;
}

t_config* cargarArchivoDeConfiguracion() {
	log_info(masterLogger, "Cargando archivo de configuracion del master.");
	char* path = "masterConfig.cfg";
	char cwd[1024];
	char *pathArchConfig = string_from_format("%s/%s", getcwd(cwd, sizeof(cwd)),
			path);
	t_config *config = config_create(pathArchConfig);
	free(pathArchConfig);

	if (!config) {
		log_error(masterLogger,"No se pudo cargar el archivo de configuracion.");
		//perror("[ERROR]: No se pudo cargar el archivo de configuracion.");
		exit(EXIT_FAILURE);
	}

	if (config_has_property(config, "YAMA_IP")) {
		ipYama = config_get_string_value(config, "YAMA_IP");
	}

	if (config_has_property(config, "YAMA_PUERTO")) {
		puertoYama = config_get_int_value(config, "YAMA_PUERTO");
	}

	if (config_has_property(config, "ID_MASTER")) {
		idMaster = config_get_int_value(config, "ID_MASTER");
	}

	log_info(masterLogger, "Archivo de configuracion cargado exitosamente");
	printf("ID Master: %d\n", idMaster);
	printf("Puerto: %d\n", puertoYama);
	printf("IP YAMA: %s\n", ipYama);
	return config;
}

void iniciarMaster(char* rutaTransformador, char* rutaReductor,
	char* archivoAprocesar, char* direccionDeResultado) {
	clock_t t_ini, t_fin;
	t_ini = clock();
	crearListas();
	crearLogger();
	t_config* config = cargarArchivoDeConfiguracion();

	socketYama = conectarseAYama(puertoYama, ipYama);

	mandarArchivosAYama(socketYama, archivoAprocesar);

	recibirPlanificacionDeYama(socketYama);

	operarEtapas();

	log_info(masterLogger, "Finaliza la ejecucion del job");
	t_fin = clock();
	double tiempo = (double) (t_fin - t_ini) / CLOCKS_PER_SEC * 1000;
	metricas(tiempo);

	//destruirListas();
	config_destroy(config);
	log_destroy(masterLogger);
}

int conectarseAYama(int puerto, char* ip) {
	struct sockaddr_in direccionYama;

	direccionYama.sin_family = AF_INET;
	direccionYama.sin_port = htons(puerto);
	direccionYama.sin_addr.s_addr = inet_addr(ip);

	int yama = socket(AF_INET, SOCK_STREAM, 0);

	if (connect(yama, (struct sockaddr *) &direccionYama,
			sizeof(struct sockaddr)) != 0) {
		log_error(masterLogger, "No se pudo conectar a YAMA");
		exit(1);
	}

	log_info(masterLogger, "Establecida conexion con YAMA");

	return yama;
}

void mandarArchivosAYama(int socketYama, char* archivoAprocesar) {
	t_header header;
	int tamanioBuffer, desplazamiento = 0;
	void * buffer, *bufferStruct;

	bufferStruct = serializarArchivos(&tamanioBuffer);
	buffer = malloc(tamanioBuffer + sizeof(t_header));
	header.id = PEDIDOTRANSFORMACION;
	header.tamanioPayload = tamanioBuffer;

	memcpy(buffer, &header.id, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, &header.tamanioPayload, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, bufferStruct, header.tamanioPayload);
	desplazamiento += header.tamanioPayload;

	enviarPorSocket(socketYama, buffer, header.tamanioPayload);
	free(bufferStruct);
	free(buffer);
	log_info(masterLogger, "Se envio las ruta a procesar a YAMA:\n%s\n%s",archivoAprocesar,direccionDeResultado);
}

void* serializarArchivos(int* largoBuffer) {
	void* buffer;
	int desplazamiento = 0, tamanioBuffer;
	t_pedidoTransformacion pedido;
	pedido.largoArchivo = strlen(archivoAprocesar)+1;
	pedido.largoArchivo2 = strlen(direccionDeResultado)+1;
	pedido.nombreArchivo = string_duplicate(archivoAprocesar);
	//pedido.nombreArchivo[pedido.largoArchivo] = "\0";
	printf("%s %d\n",pedido.nombreArchivo, pedido.largoArchivo);
	pedido.nombreArchivoGuardadoFinal = string_duplicate(direccionDeResultado);
	//pedido.nombreArchivoGuardadoFinal[pedido.largoArchivo2] = "\0";
	printf("%s %d\n",pedido.nombreArchivoGuardadoFinal, pedido.largoArchivo2);
	tamanioBuffer = pedido.largoArchivo + pedido.largoArchivo2
			+ 2 * sizeof(uint32_t);
	//tamanioBuffer = strlen(archivoAprocesar) + strlen(direccionDeResultado) + 2 * sizeof(uint32_t) + sizeof(idMaster);
	buffer = malloc(tamanioBuffer);
	//memcpy(buffer, &idMaster, sizeof(uint32_t));
	//desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, &pedido.largoArchivo, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, pedido.nombreArchivo, pedido.largoArchivo);
	desplazamiento += pedido.largoArchivo;
	memcpy(buffer + desplazamiento, &pedido.largoArchivo2, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, pedido.nombreArchivoGuardadoFinal,
			pedido.largoArchivo2);
	desplazamiento += pedido.largoArchivo2;

	*largoBuffer = tamanioBuffer;
	free(pedido.nombreArchivo);
	free(pedido.nombreArchivoGuardadoFinal);
	return buffer;
}

void recibirPlanificacionDeYama(int socketYama) {
	void* buffer;
	t_header header;
	recibirHeader(socketYama, &header);
//	if (header.id == ABORTARJOB)
	//	abortarMaster();
	if (header.id != ENVIOPLANIFICACION)
		exit(0);
	buffer = malloc(header.tamanioPayload);
	recibirPorSocket(socketYama, buffer, header.tamanioPayload);
	deserializarPlanificacion(buffer);

	log_info(masterLogger, "recibe la preplanificacion del job exitosamente.");
	free(buffer);
}

void deserializarPlanificacion(void* buffer) {

	uint32_t cantTransformaciones;
	uint32_t cantRedLocal;
	uint32_t cantRedGlobal;
	int desplazamiento = 0;
	memcpy(&cantTransformaciones, buffer, sizeof(cantTransformaciones));
	desplazamiento += sizeof(cantTransformaciones);
	memcpy(&cantRedLocal, buffer + desplazamiento, sizeof(cantRedLocal));
	desplazamiento += sizeof(cantRedLocal);
	memcpy(&cantRedGlobal, buffer + desplazamiento, sizeof(cantRedGlobal));
	desplazamiento += sizeof(cantRedGlobal);

	printf("cantTransformaciones: %d  cantRedLocal: %d cantRedGlobal: %d\n",
			cantTransformaciones, cantRedLocal, cantRedGlobal);

	deserializarTransformaciones(cantTransformaciones, buffer, &desplazamiento);

	deserializarReduccionesLocales(cantRedLocal, buffer, &desplazamiento);

	deserializarReduccionesGlobales(cantRedGlobal, buffer, &desplazamiento);
}

void deserializarTransformaciones(int cantTransformaciones, void* buffer,
		int* desplazamiento) {
	int i;
	t_transformacionMaster* transformaciones;

	for (i = 0; i < cantTransformaciones; i++) {
		transformaciones = malloc(sizeof(t_transformacionMaster));
		memcpy(&transformaciones->idNodo, buffer + *desplazamiento,
				sizeof(transformaciones->idNodo));
		*desplazamiento += sizeof(transformaciones->idNodo);
		memcpy(&transformaciones->nroBloqueNodo, buffer + *desplazamiento,
				sizeof(transformaciones->nroBloqueNodo));
		*desplazamiento += sizeof(transformaciones->nroBloqueNodo);
		memcpy(&transformaciones->bytesOcupados, buffer + *desplazamiento,
				sizeof(transformaciones->bytesOcupados));
		*desplazamiento += sizeof(transformaciones->bytesOcupados);
		memcpy(&transformaciones->puerto, buffer + *desplazamiento,
				sizeof(transformaciones->puerto));
		*desplazamiento += sizeof(transformaciones->puerto);
		memcpy(&transformaciones->largoIp, buffer + *desplazamiento,
				sizeof(transformaciones->largoIp));
		*desplazamiento += sizeof(transformaciones->largoIp);
		transformaciones->ip = malloc(transformaciones->largoIp);
		memcpy(transformaciones->ip, buffer + *desplazamiento,
				transformaciones->largoIp);
		*desplazamiento += transformaciones->largoIp;
		memcpy(&transformaciones->largoArchivo, buffer + *desplazamiento,
				sizeof(transformaciones->largoArchivo));
		*desplazamiento += sizeof(transformaciones->largoArchivo);

		transformaciones->archivoTransformacion = malloc(
				transformaciones->largoArchivo);
		memcpy(transformaciones->archivoTransformacion,
				buffer + *desplazamiento, transformaciones->largoArchivo);
		*desplazamiento += transformaciones->largoArchivo;
		//transformaciones->archivoTransformacion[transformaciones->largoArchivo]='\0';
		//transformaciones->archivoTransformacion[transformaciones->largoArchivo];
		printf("ip nodo: %s\n",transformaciones->ip);
		list_add(listaTransformaciones, transformaciones);
	}
}

void deserializarReduccionesLocales(int cantRedLocales, void* buffer,
		int* desplazamiento) {
	int i;
	t_reduccionLocalMaster* reducciones;

	for (i = 0; i < cantRedLocales; i++) {
		reducciones = malloc(sizeof(t_reduccionLocalMaster));
		memcpy(&reducciones->idNodo, buffer + *desplazamiento,
				sizeof(reducciones->idNodo));
		*desplazamiento += sizeof(reducciones[i].idNodo);
		memcpy(&reducciones->puerto, buffer + *desplazamiento,
				sizeof(reducciones->puerto));
		*desplazamiento += sizeof(reducciones->puerto);

		memcpy(&reducciones->largoIp, buffer + *desplazamiento,
				sizeof(reducciones->largoIp));
		*desplazamiento += sizeof(reducciones->largoIp);
		reducciones->ip = malloc(reducciones->largoIp + 1);
		memcpy(reducciones->ip, buffer + *desplazamiento, reducciones->largoIp);
		*desplazamiento += reducciones->largoIp;
		reducciones->ip[reducciones->largoIp] = '\0';

		memcpy(&reducciones->largoArchivoTransformacion,
				buffer + *desplazamiento,
				sizeof(reducciones->largoArchivoTransformacion));
		*desplazamiento += sizeof(reducciones->largoArchivoTransformacion);
		reducciones->archivoTransformacion = malloc(
				reducciones->largoArchivoTransformacion + 1);
		memcpy(reducciones->archivoTransformacion, buffer + *desplazamiento,
				reducciones->largoArchivoTransformacion);
		*desplazamiento += reducciones->largoArchivoTransformacion;
		reducciones->archivoTransformacion[reducciones->largoArchivoTransformacion] =
				'\0';

		memcpy(&reducciones->largoArchivoRedLocal, buffer + *desplazamiento,
				sizeof(reducciones->largoIp));
		*desplazamiento += sizeof(reducciones->largoArchivoRedLocal);
		reducciones->archivoRedLocal = malloc(
				reducciones->largoArchivoRedLocal + 1);
		memcpy(reducciones->archivoRedLocal, buffer + *desplazamiento,
				reducciones->largoArchivoRedLocal);
		*desplazamiento += reducciones->largoArchivoRedLocal;
		reducciones->archivoRedLocal[reducciones->largoArchivoRedLocal] = '\0';

		list_add(listaRedLocales, reducciones);
	}
}

void deserializarReduccionesGlobales(int cantRedGlobales, void* buffer,
		int* desplazamiento) {
	int i;

	t_reduccionGlobalMaster* reduccionesGlobales;

	for (i = 0; i < cantRedGlobales; i++) {
		reduccionesGlobales = malloc(sizeof(t_reduccionGlobalMaster));
		memcpy(&reduccionesGlobales->idNodo, buffer + *desplazamiento,
				sizeof(reduccionesGlobales->idNodo));
		*desplazamiento += sizeof(reduccionesGlobales->idNodo);
		memcpy(&reduccionesGlobales->encargado, buffer + *desplazamiento,
				sizeof(reduccionesGlobales->encargado));
		*desplazamiento += sizeof(reduccionesGlobales->encargado);
		memcpy(&reduccionesGlobales->puerto, buffer + *desplazamiento,
				sizeof(reduccionesGlobales->puerto));
		*desplazamiento += sizeof(reduccionesGlobales->puerto);
		memcpy(&reduccionesGlobales->largoIp, buffer + *desplazamiento,
				sizeof(reduccionesGlobales->largoIp));
		*desplazamiento += sizeof(reduccionesGlobales->largoIp);
		reduccionesGlobales->ip = malloc(reduccionesGlobales->largoIp + 1);
		memcpy(reduccionesGlobales->ip, buffer + *desplazamiento,
				reduccionesGlobales->largoIp);
		*desplazamiento += reduccionesGlobales->largoIp;
		reduccionesGlobales->ip[reduccionesGlobales->largoIp] = '\0';

		memcpy(&reduccionesGlobales->largoArchivoRedLocal,
				buffer + *desplazamiento, sizeof(reduccionesGlobales->largoIp));
		*desplazamiento += sizeof(reduccionesGlobales->largoIp);
		reduccionesGlobales->archivoRedLocal = malloc(
				reduccionesGlobales->largoArchivoRedLocal + 1);
		memcpy(reduccionesGlobales->archivoRedLocal, buffer + *desplazamiento,
				reduccionesGlobales->largoArchivoRedLocal);
		*desplazamiento += reduccionesGlobales->largoArchivoRedLocal;
		reduccionesGlobales->archivoRedLocal[reduccionesGlobales->largoArchivoRedLocal] =
				'\0';
		memcpy(&reduccionesGlobales->largoArchivoRedGlobal,
				buffer + *desplazamiento,
				sizeof(reduccionesGlobales->largoArchivoRedGlobal));
		*desplazamiento += sizeof(reduccionesGlobales->largoArchivoRedGlobal);
		reduccionesGlobales->archivoRedGlobal = malloc(
				reduccionesGlobales->largoArchivoRedGlobal + 1);
		memcpy(reduccionesGlobales->archivoRedGlobal, buffer + *desplazamiento,
				reduccionesGlobales->largoArchivoRedGlobal);
		*desplazamiento += reduccionesGlobales->largoArchivoRedGlobal;
		reduccionesGlobales->archivoRedGlobal[reduccionesGlobales->largoArchivoRedGlobal] =
				'\0';

		list_add(listaRedGloblales, reduccionesGlobales);
	}
}

void cargarNodosTransformacion() {
	int i, j;
	int transformaciones = list_size(listaTransformaciones);
	int redGlobales = list_size(listaRedGloblales);
	t_transformacionMaster* tmaster;
	for (i = 0; i < transformaciones; i++) {
		tmaster = list_get(listaTransformaciones, i);
		for (j = 0; j < redGlobales; j++) {
			if (nodosTransformacion[j].idNodo == tmaster->idNodo)
				nodosTransformacion[j].cantidadTransformaciones++;

			if (nodosTransformacion[j].idNodo == 0) {
				nodosTransformacion[j].idNodo = tmaster->idNodo;
				nodosTransformacion[j].cantidadTransformaciones = 1;
				j = redGlobales;
			}
		}
	}
}

void operarEtapas() {

	int i;
	replanificado = false;
	int transformaciones = list_size(listaTransformaciones);
	//int redLocales = list_size(listaRedLocales);
	int redGlobales = list_size(listaRedGloblales);
	t_reduccionGlobalMaster* reduccionGlobal;
	int finaliza = 0;
	tiempoTotalTransformaciones = 0;
	tiempoTotalRedLocales = 0;
	cantidadReduccionesLocalesRealizadas = 0;
	cantidadTransformacionesRealizadas = 0;
	reduccionGlobalRealizada = 0;
	reduccionesLocalesPendientes = redGlobales;

	inicializarMutex();

	log_info(masterLogger,"inicia el envio de tareas a los workers planificados.");

	//configuro para que libere los recursos el hilo al finalizar
	pthread_attr_t atributos;
	pthread_attr_init(&atributos);
	pthread_attr_setdetachstate(&atributos, PTHREAD_CREATE_DETACHED);

	nodosTransformacion = malloc(
			sizeof(t_transformacionesNodo) * transformaciones);

	for (i = 0; i < transformaciones; i++) {
		nodosTransformacion[i].idNodo = 0;
		nodosTransformacion[i].cantidadTransformaciones = 0;
	}
	cargarNodosTransformacion();
	enviarTransformacionAWorkers(transformador, reductor);

	t_header headerRed;
	while (finaliza == 0) {
		//recibirPorSocket(socketYama, &headerRed, sizeof(t_header));
		recibirHeader(socketYama, &headerRed);
		switch(headerRed.id){

		case REPLANIFICACION:
			log_info(masterLogger,"entra a replanificar: payload %d\n", headerRed.tamanioPayload);
			replanificarTransformaciones(headerRed.tamanioPayload);
			break;
		case INICIOREDUCCIONLOCAL:
			for (i = 0; i < list_size(listaRedGloblales); i++) {

				reduccionGlobal = (t_reduccionGlobalMaster*) list_get(listaRedGloblales, i);
				if(reduccionGlobal->idNodo == headerRed.tamanioPayload){
					pthread_t hiloConexionReduccionWorker;
					pthread_create(&hiloConexionReduccionWorker, &atributos,(void*) enviarRedLocalesAWorker,
							reduccionGlobal);
					log_info("arranco reduccion local del nodo %d",reduccionGlobal->idNodo);
				}

			}
			break;
		case INICIOREDUCCIONGLOBAL:
			printf("inicia reduccion global\n");
			log_info(masterLogger,"Inicia reduccion global");

			enviarReduccionGlobalAWorkerEncargado();
			break;
		case ALMACENAFINALYAMA:
			printf("%d\n",headerRed.id);
			finaliza = 1;
			avisarAlmacenadoFinal();
			break;
		case ABORTARJOB:
			printf("[ERROR] aborta la ejecucion del job del proceso master\n");
			log_error(masterLogger,"finaliza ejecucion del job del master de forma abrupta");
			finaliza = 1;
			break;
		default:
			break;
		}
	}

	//free(nodosTransformacion);
}

void replanificarTransformaciones(uint32_t tamanio){
	limpiarListas();
	void* buffer = malloc(tamanio);
	recibirPorSocket(socketYama, buffer, tamanio);
	deserializarPlanificacion(buffer);
	printf("replanificada la transformacion del job.\n");
	log_info(masterLogger,"replanificada la transformacion del job.");
	cargarNodosTransformacion();
	enviarTransformacionAWorkers(transformador, reductor);
	replanificado = false;
	free(buffer);

}

void enviarTransformacionAWorkers(char* rutaTransformador, char* rutaReductor) {

	int i;
	//t_header headerResp;
	t_transformacionMaster* transformacion;
	transformacionesPendientes = list_size(listaTransformaciones);
	//double tiemposTransformacion[transformacionesPendientes];

	pthread_attr_t atributos;

	pthread_attr_init(&atributos);
	pthread_attr_setdetachstate(&atributos, PTHREAD_CREATE_DETACHED);

	printf("enviar etapa de transformacion: \n");
	for (i = 0; i < list_size(listaTransformaciones); i++) {
		transformacion = (t_transformacionMaster*) list_get(
				listaTransformaciones, i);

		if(transformacionExistente(transformacion->archivoTransformacion) == 0){
			pthread_t hiloConexionesWorker;
			pthread_create(&hiloConexionesWorker, &atributos,
				(void*) hiloConexionWorker, transformacion);
		//	pthread_join(hiloConexionesWorker, NULL);

		}
	}

}

void enviarRedLocalesAWorker(t_reduccionGlobalMaster* nodoReduccion) {
	clock_t t_ini, t_fin;
	t_ini = clock();
	int i;
	t_reduccionLocalMaster redLocalMaster;
	t_infoReduccionesLocales* redLocalWorker =  malloc(sizeof(t_infoReduccionesLocales));
	t_temporalesTransformacionWorker* temporales;
	t_header header;
	printf("redLocal: nodo %d puerto %d\n", nodoReduccion->idNodo, nodoReduccion->puerto);

	pthread_mutex_lock(&mutexMaximasTareas);
	cantidadTareasCorriendoRedLocal++;
	if (cantidadTareasCorriendoRedLocal > maximoTareasCorriendoRedLocal)
		maximoTareasCorriendoRedLocal++;
	pthread_mutex_unlock(&mutexMaximasTareas);

	redLocalWorker->largoRutaArchivoReducidoLocal = nodoReduccion->largoArchivoRedLocal;
	printf("%d\n", redLocalWorker->largoRutaArchivoReducidoLocal);
	redLocalWorker->rutaArchivoReducidoLocal = malloc(redLocalWorker->largoRutaArchivoReducidoLocal);
	strcpy(redLocalWorker->rutaArchivoReducidoLocal,nodoReduccion->archivoRedLocal);
	redLocalWorker->largoArchivoReductor = devolverTamanioArchivo(reductor);
	redLocalWorker->archivoReductor = obtenerContenidoArchivo(reductor);
	redLocalWorker->temporalesTranformacion = list_create();

	for (i = 0; i < list_size(listaRedLocales); i++) {
		redLocalMaster = *(t_reduccionLocalMaster*) list_get(listaRedLocales,
				i);
		if (redLocalMaster.idNodo == nodoReduccion->idNodo) {
			temporales = malloc(sizeof(t_temporalesTransformacionWorker));
			temporales->largoRutaTemporalTransformacion =
					redLocalMaster.largoArchivoTransformacion;
			temporales->rutaTemporalTransformacion = malloc(
					temporales->largoRutaTemporalTransformacion + 1);
			strcpy(temporales->rutaTemporalTransformacion,
					redLocalMaster.archivoTransformacion);
			list_add(redLocalWorker->temporalesTranformacion, temporales);
		}
	}
	redLocalWorker->cantidadTransformaciones = list_size(
			redLocalWorker->temporalesTranformacion);

	int socketWorker = conectarseAWorker(nodoReduccion->puerto,
			nodoReduccion->ip);

	if (socketWorker == -1) {
		printf("envio desconexion del nodo a yama en el socket %d\n",
				socketYama);
		header.id = ERRORREDUCCION;
		avisarAYamaRedLocal(*redLocalWorker, header);
		pthread_mutex_lock(&mutexTotalFallos);
		fallos++;
		pthread_mutex_unlock(&mutexTotalFallos);
		log_error(masterLogger, "no se pudo conectar al worker %d en reduccion local: socket desconectado", redLocalMaster.idNodo);
	}

	else {
		int tamanioMensaje, largoBuffer, desplazamiento = 0;
		void* bufferMensaje;
		void* buffer;
		buffer = serializarReduccionLocalWorker(redLocalWorker, &largoBuffer);
		tamanioMensaje = largoBuffer + sizeof(t_header);
		bufferMensaje = malloc(tamanioMensaje);
		header.id = PEDIDOREDLOCALWORKER;
		header.tamanioPayload = largoBuffer;
		memcpy(bufferMensaje, &header.id, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(bufferMensaje + desplazamiento, &header.tamanioPayload,
				sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(bufferMensaje + desplazamiento, buffer, header.tamanioPayload);

		enviarPorSocket(socketWorker, bufferMensaje, largoBuffer);
		free(buffer);
		free(bufferMensaje);
		printf("enviado a worker la reduccion local\n");
		log_info(masterLogger,
				"Se envia operacion de reduccion local al worker %d.",
				nodoReduccion->idNodo);

		int respuesta = respuestaWorker(socketWorker);
		if (respuesta == REDUCCIONLOCALOKWORKER) {
				printf("reduccion Local OK worker %d\n",nodoReduccion->idNodo);
				header.id = REDUCCIONLOCALOKYAMA;
				pthread_mutex_lock(&mutexTotalReduccionesLocales);
				reduccionesLocalesPendientes--;
				pthread_mutex_unlock(&mutexTotalReduccionesLocales);
				avisarAYamaRedLocal(*redLocalWorker, header);
			}
		if (respuesta == 107){
			header.id = ERRORREDUCCION;
			pthread_mutex_lock(&mutexTotalReduccionesLocales);
			reduccionesLocalesPendientes--;
			pthread_mutex_unlock(&mutexTotalReduccionesLocales);
			avisarAYamaRedLocal(*redLocalWorker, header);
		}

		for (i = 0; i < list_size(redLocalWorker->temporalesTranformacion);i++) {
			free(((t_temporalesTransformacionWorker*) list_get(redLocalWorker->temporalesTranformacion, i))->rutaTemporalTransformacion);
		}
		list_destroy_and_destroy_elements(redLocalWorker->temporalesTranformacion, free);
		free(redLocalWorker->archivoReductor);
	}
	pthread_mutex_lock(&mutexMaximasTareas);
	cantidadTareasCorriendoRedLocal--;
	cantidadReduccionesLocalesRealizadas++;
	pthread_mutex_unlock(&mutexMaximasTareas);

	t_fin = clock();
	double secs = (double) (t_fin - t_ini) / CLOCKS_PER_SEC * 1000;
	pthread_mutex_lock(&mutexTiempoReducciones);
	tiempoTotalRedLocales += secs;
	pthread_mutex_unlock(&mutexTiempoReducciones);
}

void enviarReduccionGlobalAWorkerEncargado() {
	int i, socketWorkerEncargado, nodoEncargado;
	clock_t t_ini, t_fin;
	t_infoReduccionGlobal* redGlobalWorker = malloc(sizeof(t_infoReduccionGlobal));
	t_reduccionGlobalMaster* redGlobalMaster;
	t_datosNodoAEncargado* nodoWorker;
	t_header header;
	t_ini = clock();
	redGlobalWorker->largoArchivoReductor = devolverTamanioArchivo(reductor);
	redGlobalWorker->archivoReductor = obtenerContenidoArchivo(reductor);
	redGlobalWorker->nodosAConectar = list_create();

	//log_info(masterLogger,"inicia reduccion global");

	for (i = 0; i < list_size(listaRedGloblales); i++) {
		redGlobalMaster = list_get(listaRedGloblales, i);
		if (redGlobalMaster->encargado == 1) {
			printf("nodo encargado: %d\n",redGlobalMaster->idNodo);
			redGlobalWorker->largoRutaArchivoTemporalGlobal =	redGlobalMaster->largoArchivoRedGlobal;
			redGlobalWorker->rutaArchivoTemporalGlobal = malloc(redGlobalWorker->largoRutaArchivoTemporalGlobal);
			strcpy(redGlobalWorker->rutaArchivoTemporalGlobal,	redGlobalMaster->archivoRedGlobal);
			redGlobalWorker->largoRutaArchivoTemporalLocal = redGlobalMaster->largoArchivoRedLocal;
			redGlobalWorker->rutaArchivoTemporalLocal= malloc(redGlobalWorker->largoRutaArchivoTemporalLocal);
			strcpy(redGlobalWorker->rutaArchivoTemporalLocal,	redGlobalMaster->archivoRedLocal);
			socketWorkerEncargado = conectarseAWorker(redGlobalMaster->puerto,redGlobalMaster->ip);
			nodoEncargado = redGlobalMaster->idNodo;
		} else {
			nodoWorker = malloc(sizeof(t_datosNodoAEncargado));
			nodoWorker->largoIp = redGlobalMaster->largoIp;
			nodoWorker->ip = malloc(nodoWorker->largoIp);
			strcpy(nodoWorker->ip, redGlobalMaster->ip);
			nodoWorker->puerto = redGlobalMaster->puerto;
			nodoWorker->largoRutaArchivoReduccionLocal =
					redGlobalMaster->largoArchivoRedLocal+1;
			nodoWorker->rutaArchivoReduccionLocal = malloc(
					nodoWorker->largoRutaArchivoReduccionLocal);
			strcpy(nodoWorker->rutaArchivoReduccionLocal,
					redGlobalMaster->archivoRedLocal);
			string_append(&nodoWorker->rutaArchivoReduccionLocal,"\0");

			list_add(redGlobalWorker->nodosAConectar, nodoWorker);
		}
		redGlobalWorker->cantidadNodos = list_size(
				redGlobalWorker->nodosAConectar);
	}


	if (socketWorkerEncargado == -1) {
		printf("envio desconexion del nodo a yama en el socket %d\n",
				socketYama);
		header.id = ERRORREDUCCION;
		header.tamanioPayload = 0;
		avisarAYamaRedGlobal(*redGlobalWorker, header);
		//enviarPorSocket(socketYama, &header, sizeof(t_header));
		fallos++;
		log_error(masterLogger,"no se pudo conectar al worker encargado de la reduccion global.");
	} else {
		//log_info(masterLogger,"Se envia operacion de reduccion global al worker %d.",	nodoEncargado);

		int tamanioMensaje, largoBuffer, desplazamiento = 0;
		void* bufferMensaje;
		void* buffer;
		buffer = serializarReduccionGlobalWorker(redGlobalWorker,&largoBuffer);
		tamanioMensaje = largoBuffer + sizeof(t_header);
		bufferMensaje = malloc(tamanioMensaje);
		header.id = PEDIDOREDGLOBALWORKER;
		header.tamanioPayload = largoBuffer;

		memcpy(bufferMensaje, &header.id, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(bufferMensaje + desplazamiento, &header.tamanioPayload,
				sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(bufferMensaje + desplazamiento, buffer, header.tamanioPayload);

		enviarPorSocket(socketWorkerEncargado, bufferMensaje, header.tamanioPayload);
		printf("envio mensaje de reduccion global a worker\n");
		free(buffer);
		free(bufferMensaje);
		int respuesta = respuestaWorker(socketWorkerEncargado);
		if(respuesta == REDUCCIONGLOBALOKWORKER){
			header.id = REDUCCIONGLOBALOKYAMA;
			printf("enviado a worker la reduccion global\n");
			log_info(masterLogger,"Se completa la operacion de reduccion global al worker encargado");
		if(respuesta == ERRORREDUCCIONGLOBAL){
			header.id = ERRORREDUCCION;
			printf("fallo la reduccion global\n");
			log_error(masterLogger,"Fallo la operacion de reduccion global al worker %d",nodoEncargado);
		}
		avisarAYamaRedGlobal(*redGlobalWorker, header);

		reduccionGlobalRealizada++;
		}
		for (i = 0; i < list_size(redGlobalWorker->nodosAConectar); i++) {
			free(((t_datosNodoAEncargado*) list_get(redGlobalWorker->nodosAConectar, i))->ip);
			free(((t_datosNodoAEncargado*) list_get(redGlobalWorker->nodosAConectar, i))->rutaArchivoReduccionLocal);
		}
		list_destroy_and_destroy_elements(redGlobalWorker->nodosAConectar, free);
	}

	free(redGlobalWorker->archivoReductor);
	free(redGlobalWorker->rutaArchivoTemporalGlobal);
	t_fin = clock();
	tiempoTotalRedGlobal = (double) (t_fin - t_ini) / CLOCKS_PER_SEC * 1000;

}

int transformacionExistente(char* temporal){
	int i;
	char* tempGuardado;
	for(i=0;i<list_size(archivosTranformacionOk);i++){
		tempGuardado = list_get(archivosTranformacionOk, i);
		if(string_equals_ignore_case(tempGuardado, temporal)){
			return 1;
		}
	}
	return 0;
}

void avisarAYama(t_transformacionMaster* transformacion, t_header headerResp) {
	int desplazamiento;
	headerResp.tamanioPayload = transformacion->largoArchivo;
	void* buffer = malloc(sizeof(t_header) + headerResp.tamanioPayload);
	desplazamiento = 0;
	memcpy(buffer, &headerResp.id, sizeof(headerResp.id));
	desplazamiento += sizeof(headerResp.id);
	memcpy(buffer + desplazamiento, &headerResp.tamanioPayload,
			sizeof(headerResp.tamanioPayload));
	desplazamiento += sizeof(headerResp.tamanioPayload);
	memcpy(buffer + desplazamiento, transformacion->archivoTransformacion,
			headerResp.tamanioPayload);
	desplazamiento += headerResp.tamanioPayload;

	enviarPorSocket(socketYama, buffer, headerResp.tamanioPayload);
	free(buffer);
}

void avisarAYamaRedLocal(t_infoReduccionesLocales reduccionWorker,
		t_header headerResp) {
	int desplazamiento;
	headerResp.tamanioPayload = reduccionWorker.largoRutaArchivoReducidoLocal;
	void* buffer = malloc(sizeof(t_header) + headerResp.tamanioPayload);
	desplazamiento = 0;
	memcpy(buffer, &headerResp.id, sizeof(headerResp.id));
	desplazamiento += sizeof(headerResp.id);
	memcpy(buffer + desplazamiento, &headerResp.tamanioPayload,
			sizeof(headerResp.tamanioPayload));
	desplazamiento += sizeof(headerResp.tamanioPayload);
	memcpy(buffer + desplazamiento, reduccionWorker.rutaArchivoReducidoLocal,
			headerResp.tamanioPayload);
	desplazamiento += headerResp.tamanioPayload;

	enviarPorSocket(socketYama, buffer, headerResp.tamanioPayload);
	free(buffer);
}

void avisarAYamaRedGlobal(t_infoReduccionGlobal redGlobalWorker,
		t_header headerResp) {
	int desplazamiento;
	headerResp.tamanioPayload = redGlobalWorker.largoRutaArchivoTemporalGlobal;
	void* buffer = malloc(sizeof(t_header) + headerResp.tamanioPayload);
	desplazamiento = 0;
	memcpy(buffer, &headerResp.id, sizeof(headerResp.id));
	desplazamiento += sizeof(headerResp.id);
	memcpy(buffer + desplazamiento, &headerResp.tamanioPayload,
			sizeof(headerResp.tamanioPayload));
	desplazamiento += sizeof(headerResp.tamanioPayload);
	memcpy(buffer + desplazamiento, redGlobalWorker.rutaArchivoTemporalGlobal,
			headerResp.tamanioPayload);
	desplazamiento += headerResp.tamanioPayload;

	enviarPorSocket(socketYama, buffer, headerResp.tamanioPayload);
	free(buffer);
}

void avisarAlmacenadoFinal() {
	int i, socketWorker, nodoEncargado;
	t_reduccionGlobalMaster* redGlobalMaster;
	t_header header;
	char* temporalGlobal;

	for (i = 0; i < list_size(listaRedGloblales); i++) {
		redGlobalMaster = list_get(listaRedGloblales, i);
		if (redGlobalMaster->encargado == 1) {
			socketWorker = conectarseAWorker(redGlobalMaster->puerto,
					redGlobalMaster->ip);
			nodoEncargado = redGlobalMaster->idNodo;
			temporalGlobal = malloc (strlen(redGlobalMaster->archivoRedGlobal));
			strcpy(temporalGlobal, redGlobalMaster->archivoRedGlobal);
		}
	}

	if (socketWorker == -1) {
		header.id = ERRORALMACENADOFINAL;
		header.tamanioPayload = 0;
		enviarPorSocket(socketYama, &header, 0);
		fallos++;
		log_error(masterLogger,
				"no se pudo conectar al worker encargado de guardar el archivo final.");
	}

	else{
		void* buffer, *bufferStruct;
		int tamanioBuffer, desplazamiento = 0;;
		header.id = ORDENALMACENADOFINAL;
		t_infoGuardadoFinal* guardado = malloc(sizeof(t_infoGuardadoFinal));
		bufferStruct = serializarInfoGuardadoFinal(guardado,&tamanioBuffer);

		header.tamanioPayload = tamanioBuffer;
		buffer = malloc(tamanioBuffer+sizeof(t_header));
		memcpy(buffer, &header, sizeof(t_header));
		memcpy(buffer+sizeof(t_header),bufferStruct, tamanioBuffer);

		enviarPorSocket(socketWorker, buffer, tamanioBuffer);
		log_info(masterLogger,"Se avisa al worker encargado (nodo %d) para almacenar el resultado del job.",
				nodoEncargado);
		//free(temporalGlobal);
		free(bufferStruct);
		free(buffer);

		int respuesta = respuestaWorker(socketWorker);
		if(respuesta == ERRORALMACENADOFINAL){
			header.id = ERRORALMACENADOFINALYAMA;
			log_error(masterLogger,"Error en FS.");
		}
		else if(respuesta == ALMACENADOFINALOK){
			header.id = ALMACENADOFINALOK;
			log_info(masterLogger, "guardo archivo correctamente.");
			return;
		}
		header.tamanioPayload = strlen(temporalGlobal);
		buffer = malloc(sizeof(t_header) + header.tamanioPayload);


		memcpy(buffer, &header.id, sizeof(header.id));
		desplazamiento += sizeof(header.id);
		memcpy(buffer + desplazamiento, &header.tamanioPayload,
				sizeof(header.tamanioPayload));
		desplazamiento += sizeof(header.tamanioPayload);
		memcpy(buffer + desplazamiento, temporalGlobal,
				header.tamanioPayload);
		desplazamiento += header.tamanioPayload;

		enviarPorSocket(socketYama, buffer, header.tamanioPayload);
		log_info(masterLogger, "Envio a Yama que no se guardo el archivo.");
		free(buffer);
	}
}

void* serializarInfoGuardadoFinal(t_infoGuardadoFinal* guardado,int* tamanioBuffer){
	t_reduccionGlobalMaster* redGlobal;
	void* buffer;
	int i,desplazamiento=0;

	guardado->nombreArchivoTemporal = string_new();
	for(i=0;i<list_size(listaRedGloblales);i++){
		redGlobal = list_get(listaRedGloblales, i);
		if(redGlobal->encargado == 1){
			guardado->largoRutaTemporalArchivo = redGlobal->largoArchivoRedGlobal+1;
			//guardado->nombreArchivoTemporal = malloc(redGlobal->largoArchivoRedGlobal);
			string_append(&guardado->nombreArchivoTemporal,redGlobal->archivoRedGlobal);
			string_append(&guardado->nombreArchivoTemporal,"\0");
			i = listaRedGloblales->elements_count;
		}
	}
	guardado->largoRutaArchivoFinal = strlen(direccionDeResultado)+1;
	guardado->nombreArchivoArchivoFinal = malloc(guardado->largoRutaArchivoFinal);
	strcpy(guardado->nombreArchivoArchivoFinal, direccionDeResultado);
	string_append(&guardado->nombreArchivoArchivoFinal,"\0");
	buffer = malloc(sizeof(uint32_t)*2 + guardado->largoRutaArchivoFinal +
			guardado->largoRutaTemporalArchivo);

	memcpy(buffer+desplazamiento,&guardado->largoRutaTemporalArchivo,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(buffer+desplazamiento,guardado->nombreArchivoTemporal,guardado->largoRutaTemporalArchivo);
	desplazamiento += guardado->largoRutaTemporalArchivo;
	memcpy(buffer+desplazamiento,&guardado->largoRutaArchivoFinal,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento,guardado->nombreArchivoArchivoFinal,guardado->largoRutaArchivoFinal);
	desplazamiento += guardado->largoRutaArchivoFinal;

	*tamanioBuffer = desplazamiento;
	//free(guardado->nombreArchivoArchivoFinal);
	//free(guardado->nombreArchivoTemporal);
	//free(guardado);
	
	return buffer;
}

// * Hilo conexion con cada worker   * ///
void hiloConexionWorker(t_transformacionMaster* transformacion) {
	clock_t t_ini, t_fin;
	t_infoTransformacion* worker;
	void* buffer, *bufferMensaje;
	int largoBuffer, tamanioMensaje, desplazamiento = 0;
	t_header header;
	t_ini = clock();
	pthread_mutex_lock(&mutexMaximasTareas);
	printf("nodo: %d, puerto %d\n", transformacion->idNodo,
			transformacion->puerto);
	cantidadTareasCorriendoTransformacion++;
	if (cantidadTareasCorriendoTransformacion > maximoTareasCorriendoTransformacion)
		maximoTareasCorriendoTransformacion++;
	pthread_mutex_unlock(&mutexMaximasTareas);

	worker = malloc(sizeof(t_infoTransformacion));
	worker->bytesOcupados = transformacion->bytesOcupados;
	worker->bloqueATransformar = transformacion->nroBloqueNodo;
	worker->largoRutaArchivo = transformacion->largoArchivo + 1;
	worker->rutaArchivoTemporal = malloc(worker->largoRutaArchivo);
	strcpy(worker->rutaArchivoTemporal, transformacion->archivoTransformacion);
	worker->largoArchivoTransformador = devolverTamanioArchivo(transformador);
	worker->archivoTransformador = obtenerContenidoArchivo(transformador);

	pthread_mutex_lock(&mutexConexionWorker);
	int socketWorker = conectarseAWorker(transformacion->puerto,
			transformacion->ip);
	pthread_mutex_unlock(&mutexConexionWorker);
	if (socketWorker == -1) {
		printf("envio desconexion del nodo a yama en el socket %d\n",socketYama);
		pthread_mutex_lock(&mutexReplanificado);
		if(!replanificado){
			enviarFalloTransformacionAYama(transformacion, &header);
			borrarTemporalesDeNodo(transformacion->idNodo);
			replanificado = true;
		}
		pthread_mutex_unlock(&mutexReplanificado);
		log_error(masterLogger, "El worker %d no se encontro levantado.\n",transformacion->idNodo);
		borrarTemporalesDeNodo(transformacion->idNodo);
	} else {
		buffer = serializarTransformacionWorker(worker, &largoBuffer);
		tamanioMensaje = largoBuffer + sizeof(t_header);
		bufferMensaje = malloc(tamanioMensaje);
		header.id = PEDIDOTRANSFORMACIONWORKER;
		header.tamanioPayload = largoBuffer;

		memcpy(bufferMensaje, &header.id, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(bufferMensaje + desplazamiento, &header.tamanioPayload,
				sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(bufferMensaje + desplazamiento, buffer, header.tamanioPayload);

		enviarPorSocket(socketWorker, bufferMensaje, largoBuffer);
		free(buffer);
		free(bufferMensaje);
		printf("enviado a worker %d\n", transformacion->idNodo);
		log_info(masterLogger, "Enviado transformacion a worker %d",transformacion->idNodo);

		if (respuestaWorker(socketWorker) == TRANSFORMACIONOKWORKER) {
			pthread_mutex_lock(&mutexTotalTransformaciones);
			disminuirTransformacionesDeNodo(transformacion->idNodo);
			cantidadTransformacionesRealizadas++;
			list_add(archivosTranformacionOk, transformacion->archivoTransformacion);
			pthread_mutex_unlock(&mutexTotalTransformaciones);
			header.id = TRANSFORMACIONOKYAMA;
			avisarAYama(transformacion, header);
		}
		else{
			printf("error en nodo %d\n",transformacion->idNodo);
			if(!replanificado){
				enviarFalloTransformacionAYama(transformacion, &header);
				borrarTemporalesDeNodo(transformacion->idNodo);
				replanificado = true;
			}
			log_error(masterLogger, "El worker %d no pudo terminar la ejecucion de la tarea de transformacion.\n",
							transformacion->idNodo);
		}
	}
	free(worker->rutaArchivoTemporal);
	free(worker->archivoTransformador);
	free(worker);

	pthread_mutex_lock(&mutexMaximasTareas);
	cantidadTareasCorriendoTransformacion--;
	pthread_mutex_unlock(&mutexMaximasTareas);

	t_fin = clock();
	double secs = (double) (t_fin - t_ini) / CLOCKS_PER_SEC * 1000;
	pthread_mutex_lock(&mutexTiempoTransformaciones);
	tiempoTotalTransformaciones += secs;
	pthread_mutex_unlock(&mutexTiempoTransformaciones);
}

void enviarFalloTransformacionAYama(t_transformacionMaster* transformacion,
		t_header* header) {
	void* buffer;
	int desplazamiento = 0, tamanioBuffer;

	t_falloTransformacion* fallo = malloc(sizeof(t_falloTransformacion));
	fallo->largoRutaTemporal = transformacion->largoArchivo;
	fallo->rutaTemporalTransformacion = malloc(fallo->largoRutaTemporal);
	strcpy(fallo->rutaTemporalTransformacion,transformacion->archivoTransformacion);
	string_append(&fallo->rutaTemporalTransformacion,"\0");
	fallo->largoRutaArchivoAProcesar = strlen(archivoAprocesar)+1; //tercer argumento de master
	fallo->rutaArchivoAProcesar = string_duplicate(archivoAprocesar);

	fallo->largoRutaArchivoDestino = strlen(direccionDeResultado)+1; //cuarto argumento de master
	fallo->rutaArchivoDestino= string_duplicate(direccionDeResultado);

	tamanioBuffer = sizeof(t_header) + (3 * sizeof(uint32_t))
			+ fallo->largoRutaArchivoAProcesar + fallo->largoRutaTemporal + fallo->largoRutaArchivoDestino;

	buffer = malloc(tamanioBuffer);

	header->id = ERRORTRANSFORMACION;
	header->tamanioPayload = tamanioBuffer - sizeof(t_header);

	memcpy(buffer, &header->id, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, &header->tamanioPayload, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, &fallo->largoRutaTemporal, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, fallo->rutaTemporalTransformacion,
			fallo->largoRutaTemporal);

	desplazamiento += fallo->largoRutaTemporal;
	memcpy(buffer + desplazamiento, &fallo->largoRutaArchivoAProcesar,
			sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, fallo->rutaArchivoAProcesar,
			fallo->largoRutaArchivoAProcesar);
	desplazamiento += fallo->largoRutaArchivoAProcesar;
	memcpy(buffer + desplazamiento, &fallo->largoRutaArchivoDestino,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, fallo->rutaArchivoDestino,fallo->largoRutaArchivoDestino);
	desplazamiento += fallo->largoRutaArchivoDestino;

	enviarPorSocket(socketYama, buffer, header->tamanioPayload);
	pthread_mutex_lock(&mutexTotalFallos);
	fallos++;
	pthread_mutex_unlock(&mutexTotalFallos);
	free(fallo->rutaArchivoAProcesar);
	free(fallo->rutaTemporalTransformacion);
	free(fallo);
	free(buffer);
}

void borrarTemporalesDeNodo(int nodo){
	char* temporal;
	char* contenido = string_new();
	string_append(&contenido, "n");
	string_append(&contenido, string_itoa(nodo));
	int i = 0;
	while(i<list_size(archivosTranformacionOk)){
		temporal = list_get(archivosTranformacionOk,i);
		if(string_contains(temporal,contenido)){
			list_remove_and_destroy_element(archivosTranformacionOk, i, free);
		}
		else{
			i++;
		}
	}
	free(contenido);
}

void disminuirTransformacionesDeNodo(int nodo) {
	int i;
	for (i = 0; i < list_size(listaRedGloblales); i++) {
		if (nodosTransformacion[i].idNodo == nodo)
			nodosTransformacion[i].cantidadTransformaciones--;
	}
	transformacionesPendientes--;
}

int conectarseAWorker(int puerto, char* ip) {

	struct sockaddr_in direccionWorker;

	direccionWorker.sin_family = AF_INET;
	direccionWorker.sin_port = htons(puerto);
	direccionWorker.sin_addr.s_addr = inet_addr(ip);
	//memset(&(direccionYama.sin_zero), '\0', 8);
	int socketWorker;

	socketWorker = socket(AF_INET, SOCK_STREAM, 0);
	if(socketWorker <= 0){
		log_error(masterLogger, "Error en la conexion del worker.");
		return -1;
	}

	if (connect(socketWorker, (struct sockaddr *) &direccionWorker,sizeof(struct sockaddr)) != 0) {
		log_error(masterLogger,"fallo la conexion al worker");
		return -1;
	}

	return socketWorker;
}

void* serializarTransformacionWorker(t_infoTransformacion* worker,int* largoBuffer) {
	void* buffer;
	int desplazamiento = 0;
	int tamanioBuffer = worker->largoRutaArchivo
			+ worker->largoArchivoTransformador + 4 * sizeof(uint32_t);
	buffer = malloc(tamanioBuffer);
	memcpy(buffer + desplazamiento, &worker->bloqueATransformar,
			sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, &worker->bytesOcupados, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, &worker->largoRutaArchivo,
			sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(buffer + desplazamiento, worker->rutaArchivoTemporal,
			worker->largoRutaArchivo);
	desplazamiento += worker->largoRutaArchivo;

	memcpy(buffer + desplazamiento, &worker->largoArchivoTransformador,
			sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, worker->archivoTransformador,
			worker->largoArchivoTransformador);
	desplazamiento += worker->largoArchivoTransformador;

	*largoBuffer = desplazamiento;

	return buffer;
}

void* serializarReduccionLocalWorker(t_infoReduccionesLocales* redLocalWorker,int* largoBuffer) {
	int i, desplazamiento = 0, tamanioBuffer = 0;
	void* buffer;
	t_temporalesTransformacionWorker temporales;

	tamanioBuffer = 3 * sizeof(uint32_t) + redLocalWorker->largoArchivoReductor
			+ redLocalWorker->largoRutaArchivoReducidoLocal;

	buffer = malloc(tamanioBuffer);


	memcpy(buffer + desplazamiento, &redLocalWorker->largoRutaArchivoReducidoLocal, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, redLocalWorker->rutaArchivoReducidoLocal,redLocalWorker->largoRutaArchivoReducidoLocal);
	desplazamiento += redLocalWorker->largoRutaArchivoReducidoLocal;

	memcpy(buffer + desplazamiento, &redLocalWorker->largoArchivoReductor,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, redLocalWorker->archivoReductor,redLocalWorker->largoArchivoReductor);
	desplazamiento += redLocalWorker->largoArchivoReductor;

	memcpy(buffer + desplazamiento, &redLocalWorker->cantidadTransformaciones,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	for (i = 0; i < redLocalWorker->cantidadTransformaciones; i++) {
		temporales = *(t_temporalesTransformacionWorker*) (list_get(
				redLocalWorker->temporalesTranformacion, i));
		tamanioBuffer += temporales.largoRutaTemporalTransformacion
				+ sizeof(uint32_t);
		buffer = realloc(buffer, tamanioBuffer);

		memcpy(buffer + desplazamiento,&temporales.largoRutaTemporalTransformacion, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(buffer + desplazamiento, temporales.rutaTemporalTransformacion,temporales.largoRutaTemporalTransformacion);
		desplazamiento += temporales.largoRutaTemporalTransformacion;
	}


	*largoBuffer = tamanioBuffer;
	return buffer;
}

void* serializarReduccionGlobalWorker(t_infoReduccionGlobal* redGlobalWorker,
		int* largoBuffer) {
	int i, desplazamiento = 0, tamanioBuffer = 0;
	void* buffer;
	t_datosNodoAEncargado *nodos;
	tamanioBuffer = 4 * sizeof(uint32_t) + redGlobalWorker->largoArchivoReductor
			+ redGlobalWorker->largoRutaArchivoTemporalGlobal + redGlobalWorker->largoRutaArchivoTemporalLocal;
	buffer = malloc(tamanioBuffer);

	memcpy(buffer + desplazamiento, &redGlobalWorker->largoRutaArchivoTemporalLocal,
			sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, redGlobalWorker->rutaArchivoTemporalLocal,
			redGlobalWorker->largoRutaArchivoTemporalLocal);
	desplazamiento += redGlobalWorker->largoRutaArchivoTemporalLocal;


	memcpy(buffer + desplazamiento, &redGlobalWorker->largoRutaArchivoTemporalGlobal,
			sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, redGlobalWorker->rutaArchivoTemporalGlobal,
			redGlobalWorker->largoRutaArchivoTemporalGlobal);
	desplazamiento += redGlobalWorker->largoRutaArchivoTemporalGlobal;

	memcpy(buffer + desplazamiento, &redGlobalWorker->largoArchivoReductor,
			sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, redGlobalWorker->archivoReductor,
			redGlobalWorker->largoArchivoReductor);
	desplazamiento += redGlobalWorker->largoArchivoReductor;

	memcpy(buffer + desplazamiento, &redGlobalWorker->cantidadNodos,
			sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	for (i = 0; i < redGlobalWorker->cantidadNodos; i++) {
		nodos = (t_datosNodoAEncargado*) (list_get(
				redGlobalWorker->nodosAConectar, i));
		tamanioBuffer += nodos->largoRutaArchivoReduccionLocal + nodos->largoIp
				+ 3 * sizeof(uint32_t);
		buffer = realloc(buffer, tamanioBuffer);
		memcpy(buffer + desplazamiento, &nodos->puerto, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);

		memcpy(buffer + desplazamiento, &nodos->largoIp, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(buffer + desplazamiento, nodos->ip, nodos->largoIp);
		desplazamiento += nodos->largoIp;

		memcpy(buffer + desplazamiento, &nodos->largoRutaArchivoReduccionLocal,
				sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(buffer + desplazamiento, nodos->rutaArchivoReduccionLocal,
				nodos->largoRutaArchivoReduccionLocal);
		desplazamiento += nodos->largoRutaArchivoReduccionLocal;
	}

	*largoBuffer = tamanioBuffer;
	return buffer;
}

int respuestaWorker(int socketWorker) {

	char* buffer = malloc(sizeof(int));
	t_header header;
	int bytesRecibidos = recibirHeader(socketWorker, &header);
	free(buffer);
	if(bytesRecibidos == 0)
		return 0;
	else //return respuesta;
		return header.id;
}

void metricas(double tiempo) {
	double promedioTransformaciones = tiempoTotalTransformaciones
			/ (double) cantidadTransformacionesRealizadas;
	double promedioReducciones = tiempoTotalRedLocales
			/ (double) tiempoTotalRedLocales;
	printf("\n--metricas--\n\n");
/*	printf("tiempo de ejecucion del job: %.16g milisegundos\n", tiempo);
	printf("tiempo de ejecucion de etapa de transformacion: %.16g milisegundos\n", tiempoTotalTransformaciones);
	printf("tiempo de ejecucion de etapa de reduccion local: %.16g milisegundos\n", tiempoTotalRedLocales);
	printf("tiempo promedio de ejecucion de etapa de transformacion: %.16g milisegundos\n",
			promedioTransformaciones);
	printf("tiempo promedio de ejecucion de etapa de reduccion local: %.16g milisegundos\n",
			promedioReducciones);
	printf("tiempo promedio de ejecucion de etapa de reduccion global: %.16g milisegundos\n",
			tiempoTotalRedGlobal);
	printf("total de transformaciones realizadas: %d\n",cantidadTransformacionesRealizadas);
	printf("total de reducciones locales realizadas: %d\n",cantidadReduccionesLocalesRealizadas);
	printf("total de reducciones globales realizadas: %d\n",reduccionGlobalRealizada);
	printf("cantidad maxima de tareas de transformacion en paralelo: %d\n",maximoTareasCorriendoTransformacion);
	printf("cantidad maxima de tareas de reduccion local en paralelo: %d\n",maximoTareasCorriendoRedLocal);
	printf("cantidad de fallos obtenidos: %d\n", fallos);
*/
	log_info(masterLogger,"\n\t--metricas--\n");
	log_info(masterLogger,"tiempo de ejecucion del job: %.16g ms", tiempo * 1000);
	log_info(masterLogger,"tiempo promedio de ejecucion de etapa de transformacion: %.16g ms",
			promedioTransformaciones);
	log_info(masterLogger,"tiempo promedio de ejecucion de etapa de reduccion local: %.16g ms",
			promedioReducciones);
	log_info(masterLogger,"tiempo promedio de ejecucion de etapa de reduccion global: %.16g ms",
		tiempoTotalRedGlobal);
	log_info(masterLogger,"total de transformaciones realizadas: %d",
			cantidadTransformacionesRealizadas);
	log_info(masterLogger,"total de reducciones locales realizadas: %d",
			cantidadReduccionesLocalesRealizadas);
	log_info(masterLogger,"total de reducciones globales realizadas: %d",
				reduccionGlobalRealizada);
	log_info(masterLogger,"cantidad maxima de tareas de transformacion en paralelo: %d",
				maximoTareasCorriendoTransformacion);
	log_info(masterLogger,"cantidad maxima de tareas de reduccion local en paralelo: %d",maximoTareasCorriendoRedLocal);
	log_info(masterLogger,"cantidad de fallos obtenidos: %d", fallos);
}

int devolverTamanioArchivo(char* archivo) {
	int file = open(archivo, O_RDONLY);
	struct stat mystat;
	if (file == -1) {
		perror("open");
		exit(1);
	}
	if (fstat(file, &mystat) < 0) {
		perror("fstat");
		close(file);
		exit(1);
	}
	int tam = mystat.st_size;

	close(file);
	tam++;
	return tam;
}

char* obtenerContenidoArchivo(char* archivo) {
	char* buffer;
	int file = open(archivo, O_RDWR);
	struct stat mystat;
	if (file == -1) {
		log_error(masterLogger,"error al abrir el archivo %s.",archivo);
		exit(1);
	}
	if (fstat(file, &mystat) < 0) {
		log_error(masterLogger, "Error al calcular el fstat del archivo %s.",archivo);
		close(file);
		exit(1);
	}
	int tam = mystat.st_size;
	buffer = malloc(tam * sizeof(char) + 1);
	char* pmap = (char *) mmap(0, tam, PROT_READ, MAP_SHARED, file, 0);
	int i;
	for (i = 0; i < tam; i++) {
		buffer[i] = pmap[i];
	}
	buffer[i] = '\0';
	close(file);
	munmap(pmap, tam);
	return buffer;
}
