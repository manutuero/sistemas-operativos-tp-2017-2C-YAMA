/*
 * funcionesMaster.c
 *
 *  Created on: 11/9/2017
 *      Author: utnso
 */

#include "funcionesMaster.h"

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
}

void crearLogger() {
	char *pathLogger = string_new();
	char cwd[1024];
	string_append(&pathLogger, getcwd(cwd, sizeof(cwd)));
	string_append(&pathLogger, "/masterLogs.log");

	char *logMasterFileName = strdup("masterLogs.log");
	masterLogger = log_create(pathLogger, logMasterFileName, false,
			LOG_LEVEL_INFO);
	free(logMasterFileName);
	free(pathLogger);
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
		perror("[ERROR]: No se pudo cargar el archivo de configuracion.");
		exit(EXIT_FAILURE);
	}

	if (config_has_property(config, "YAMA_IP")) {
		ipYama = config_get_string_value(config, "YAMA_IP");
	}

	if (config_has_property(config, "YAMA_PUERTO")) {
		puertoYama = config_get_int_value(config, "YAMA_PUERTO");
	}

	log_info(masterLogger, "Archivo de configuracion cargado exitosamente");
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
	double tiempo = (double) (t_fin - t_ini) / CLOCKS_PER_SEC;
	metricas(tiempo);

	destruirListas();

	config_destroy(config);
	log_destroy(masterLogger);
}

int conectarseAYama(int puerto, char* ip) {
	struct sockaddr_in direccionYama;

	direccionYama.sin_family = AF_INET;
	direccionYama.sin_port = htons(puerto);
	direccionYama.sin_addr.s_addr = inet_addr(ip);
	//memset(&(direccionYama.sin_zero), '\0', 8);

	int yama;

	yama = socket(AF_INET, SOCK_STREAM, 0);

	if (connect(yama, (struct sockaddr *) &direccionYama,
			sizeof(struct sockaddr)) != 0) {
		perror("fallo la conexion a YAMA");
		log_error(masterLogger, "No se pudo conectar a YAMA");
		exit(1);
	}

	printf("se conecto a YAMA en el socket %d\n", yama);
	log_info(masterLogger, "Establecida conexion con YAMA");

	return yama;
}

void mandarArchivosAYama(int socketYama, char* archivoAprocesar) {
	t_rutaArchivo ruta;
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
	/*ruta.tamanio = strlen(archivoAprocesar);
	 ruta.ruta = archivoAprocesar;
	 //ruta.ruta[ruta.tamanio] = '\0';
	 //int tamanioMensaje = header.tamanio + sizeof(header);
	 void* buffer;
	 buffer = serializarRutaArchivo(&header,&ruta); //esta en utils ya que lo voy a usar para Yama-fs
	 */

	int tamanioMensaje = header.tamanioPayload + sizeof(t_header);
	enviarPorSocket(socketYama, buffer, tamanioMensaje);
	free(bufferStruct);
	free(buffer);
	log_info(masterLogger, "Se envio la ruta del archivo a procesar a YAMA");
}

void* serializarArchivos(int* largoBuffer) {
	void* buffer;
	int desplazamiento = 0, tamanioBuffer;
	t_pedidoTransformacion pedido;
	pedido.largoArchivo = strlen(archivoAprocesar);
	pedido.largoArchivo2 = strlen(direccionDeResultado);
	pedido.nombreArchivo = string_duplicate(archivoAprocesar);
	pedido.nombreArchivoGuardadoFinal = string_duplicate(direccionDeResultado);

	tamanioBuffer = strlen(archivoAprocesar) + strlen(direccionDeResultado)
			+ 2 * sizeof(uint32_t);
	buffer = malloc(tamanioBuffer);
	memcpy(buffer, &pedido.largoArchivo, sizeof(uint32_t));
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
	if (header.id != ENVIOPLANIFICACION)
		exit(0);
	buffer = malloc(header.tamanioPayload);
	recibirPorSocket(socketYama, buffer, header.tamanioPayload);
	deserializarPlanificacion(buffer);
	int i;
	printf("archivos de transformacion:\n");
	for (i = 0; i < list_size(listaTransformaciones); i++) {
		printf("en lista: %s\n",
				((t_transformacionMaster*) list_get(listaTransformaciones, i))->ip);
		printf("en lista: %s\n",
				((t_transformacionMaster*) list_get(listaTransformaciones, i))->archivoTransformacion);

	}

	printf("archivos de reduccion local:\n");
	for (i = 0; i < list_size(listaRedLocales); i++) {
		printf("en lista: %s\n",
				((t_reduccionLocalMaster*) list_get(listaRedLocales, i))->archivoRedLocal);

	}

	printf("archivos de reduccion global:\n");
	for (i = 0; i < list_size(listaRedGloblales); i++) {
		printf("en lista: %s\n",
				((t_reduccionGlobalMaster*) list_get(listaRedGloblales, i))->archivoRedGlobal);

	}
	log_info(masterLogger, "recibe la preplanificacion del job exitosamente.");
	free(buffer);
}

void deserializarPlanificacion(void* buffer) {

	uint32_t cantTransformaciones;
	uint32_t cantRedLocal;
	uint32_t cantRedGlobal;
	int i, desplazamiento = 0;
	memcpy(&cantTransformaciones, buffer, sizeof(cantTransformaciones));
	desplazamiento += sizeof(cantTransformaciones);
	memcpy(&cantRedLocal, buffer + desplazamiento, sizeof(cantRedLocal));
	desplazamiento += sizeof(cantRedLocal);
	memcpy(&cantRedGlobal, buffer + desplazamiento, sizeof(cantRedGlobal));
	desplazamiento += sizeof(cantRedGlobal);

	printf("cantTransformaciones: %d  cantRedLocal: %d cantRedGlobal: %d\n",
			cantTransformaciones, cantRedLocal, cantRedGlobal);

	t_transformacionMaster transformaciones[cantTransformaciones];
	t_reduccionLocalMaster reducciones[cantRedLocal];
	t_reduccionGlobalMaster reduccionesGlobales[cantRedGlobal];

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
		transformaciones->ip = malloc(transformaciones->largoIp + 1);
		memcpy(transformaciones->ip, buffer + *desplazamiento,
				transformaciones->largoIp);
		*desplazamiento += transformaciones->largoIp;
		transformaciones->ip[transformaciones->largoIp] = '\0';
		memcpy(&transformaciones->largoArchivo, buffer + *desplazamiento,
				sizeof(transformaciones->largoArchivo));
		*desplazamiento += sizeof(transformaciones->largoArchivo);

		transformaciones->archivoTransformacion = malloc(
				transformaciones->largoArchivo + 1);
		memcpy(transformaciones->archivoTransformacion,
				buffer + *desplazamiento, transformaciones->largoArchivo);
		*desplazamiento += transformaciones->largoArchivo;
		transformaciones->archivoTransformacion[transformaciones->largoArchivo] =
				'\0';
		//free(transformaciones->ip);
		//free(transformaciones->archivoTransformacion);

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
		//free(transformaciones->ip);
		//free(transformaciones->archivoTransformacion);
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
		//free(transformaciones->ip);
		//free(transformaciones->archivoTransformacion);
	}
}

void operarEtapas() {

	int i, j;
	int transformaciones = list_size(listaTransformaciones);
	//int redLocales = list_size(listaRedLocales);
	int redGlobales = list_size(listaRedGloblales);
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

	enviarTransformacionAWorkers(transformador, reductor);

	t_header headerRed;
	uint32_t respuestaYama;
	while (finaliza == 0) {
		printf("esperando respuesta de yama\n");
		recibirPorSocket(socketYama, &respuestaYama, sizeof(uint32_t));
		switch(respuestaYama){

		case REPLANIFICACION:
			replanificarTransformaciones(headerRed.tamanioPayload);
			break;
		case INICIOREDUCCIONLOCAL:
			for (i = 0; i < list_size(listaRedGloblales); i++) {
				//if (nodosTransformacion[i].idNodo == headerRed.tamanioPayload) { //en tamanioPayload tengo el idnodo
				if(nodosTransformacion[i].cantidadTransformaciones == 0){
					log_info(masterLogger, "inicia la reduccion local del nodo %d",nodosTransformacion->idNodo);
					printf("termino todas las transformaciones del nodo %d\n",
					nodosTransformacion[i].idNodo);

					pthread_t hiloConexionReduccionWorker;
					pthread_create(&hiloConexionReduccionWorker, &atributos,(void*) enviarRedLocalesAWorker,
							(t_reduccionGlobalMaster*) list_get(listaRedGloblales, i));
					nodosTransformacion[i].cantidadTransformaciones--;
					//pthread_join(hiloConexionReduccionWorker, NULL);
				}

			}
			break;
		case INICIOREDUCCIONGLOBAL:
			enviarReduccionGlobalAWorkerEncargado();
			//finaliza = 1;
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
			printf("%d\n",headerRed.id);
			break;
		}
	}

	free(nodosTransformacion);
}

void replanificarTransformaciones(int tamanio){
	//t_list* listaAuxTransformaciones = list_create();
	//t_list* listaAuxRedLocales = list_create();
	//t_list* listaAuxRedGlobales = list_create();
	void* buffer = malloc(tamanio);
	recibirPorSocket(socketYama, buffer, tamanio);
	limpiarListas();
	deserializarPlanificacion(buffer);

	enviarTransformacionAWorkers(transformador, reductor);
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
		//transformacion = malloc(sizeof(t_transformacionMaster));
		//transformacion = malloc(sizeof(t_transformacionMaster));
		transformacion = (t_transformacionMaster*) list_get(
				listaTransformaciones, i);
		if (transformacion->idNodo == 1)
			transformacion->puerto = 6671;
		if (transformacion->idNodo == 3)
			transformacion->puerto = 6672;
		if (transformacion->idNodo == 4)
			transformacion->puerto = 6673;
		//transformacion->ip = "10.0.2.15";

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
	t_infoReduccionesLocales redLocalWorker;
	t_temporalesTransformacionWorker* temporales;
	t_header header;

	//nodoReduccion->ip = "10.0.2.15";
	if (nodoReduccion->idNodo == 1)
		nodoReduccion->puerto = 6671;
	if (nodoReduccion->idNodo == 3)
		nodoReduccion->puerto = 6672;
	if (nodoReduccion->idNodo == 4)
		nodoReduccion->puerto = 6673;

	pthread_mutex_lock(&mutexMaximasTareas);
	cantidadTareasCorriendoRedLocal++;
	if (cantidadTareasCorriendoRedLocal > maximoTareasCorriendoRedLocal)
		maximoTareasCorriendoRedLocal++;
	pthread_mutex_unlock(&mutexMaximasTareas);

	redLocalWorker.etapa = 2;
	redLocalWorker.largoRutaArchivoReductorLocal = strlen(
			nodoReduccion->archivoRedLocal);
	redLocalWorker.rutaArchivoReductorLocal = nodoReduccion->archivoRedLocal;
	redLocalWorker.largoArchivoReductor = devolverTamanioArchivo(reductor);
	redLocalWorker.archivoReductor = obtenerContenidoArchivo(reductor);
	redLocalWorker.temporalesTranformacion = list_create();

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
			list_add(redLocalWorker.temporalesTranformacion, temporales);
		}
	}
	redLocalWorker.cantidadTransformaciones = list_size(
			redLocalWorker.temporalesTranformacion);

	int socketWorker = conectarseAWorker(nodoReduccion->puerto,
			nodoReduccion->ip);

	if (socketWorker == -1) {
		printf("envio desconexion del nodo a yama en el socket %d\n",
				socketYama);
		header.id = ERRORREDUCCION;
		avisarAYamaRedLocal(redLocalWorker, header);
		//enviarPorSocket(socketYama, &header, sizeof(t_header));
		pthread_mutex_lock(&mutexTotalFallos);
		fallos++;
		pthread_mutex_unlock(&mutexTotalFallos);
		log_error(masterLogger, "no se pudo conectar al worker %d: socket desconectado", redLocalMaster.idNodo);
	}

	else {
		int tamanioMensaje, largoBuffer, desplazamiento = 0;
		void* bufferMensaje;
		void* buffer;
		buffer = serializarReduccionLocalWorker(&redLocalWorker, &largoBuffer);
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

		enviarPorSocket(socketWorker, bufferMensaje, tamanioMensaje);
		free(buffer);
		free(bufferMensaje);
		printf("enviado a worker la reduccion local\n");
		log_info(masterLogger,
				"Se envia operacion de reduccion local al worker %d.",
				nodoReduccion->idNodo);

		if (respuestaTransformacion(socketWorker) == REDUCCIONLOCALOKWORKER) {
				printf("transformacion OK\n");
				header.id = REDUCCIONLOCALOKYAMA;
				pthread_mutex_lock(&mutexTotalReduccionesLocales);
				reduccionesLocalesPendientes--;
				pthread_mutex_unlock(&mutexTotalReduccionesLocales);
				avisarAYamaRedLocal(redLocalWorker, header);
			}






		for (i = 0; i < list_size(redLocalWorker.temporalesTranformacion);
				i++) {
			free(
					((t_temporalesTransformacionWorker*) list_get(
							redLocalWorker.temporalesTranformacion, i))->rutaTemporalTransformacion);
		}
		list_destroy_and_destroy_elements(
				redLocalWorker.temporalesTranformacion, free);
		free(redLocalWorker.archivoReductor);
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
	t_infoReduccionGlobal redGlobalWorker;
	t_reduccionGlobalMaster* redGlobalMaster;
	t_datosNodoAEncargado* nodoWorker;
	t_header header;
	t_ini = clock();
	redGlobalWorker.etapa = 3;
	redGlobalWorker.largoArchivoReductor = devolverTamanioArchivo(reductor);
	redGlobalWorker.archivoReductor = obtenerContenidoArchivo(reductor);
	redGlobalWorker.nodosAConectar = list_create();

	for (i = 0; i < list_size(listaRedGloblales); i++) {
		redGlobalMaster = list_get(listaRedGloblales, i);
		if (redGlobalMaster->encargado == 1) {
			redGlobalWorker.largoRutaArchivoTemporal =
					redGlobalMaster->largoArchivoRedGlobal;
			redGlobalWorker.rutaArchivoTemporal = malloc(
					redGlobalWorker.largoRutaArchivoTemporal);
			strcpy(redGlobalWorker.rutaArchivoTemporal,
					redGlobalMaster->archivoRedGlobal);
			socketWorkerEncargado = conectarseAWorker(redGlobalMaster->puerto,
					redGlobalMaster->ip);
			nodoEncargado = redGlobalMaster->idNodo;
		} else {
			nodoWorker = malloc(sizeof(t_datosNodoAEncargado));
			nodoWorker->largoIp = redGlobalMaster->largoIp;
			nodoWorker->ip = malloc(nodoWorker->largoIp);
			strcpy(nodoWorker->ip, redGlobalMaster->ip);
			nodoWorker->puerto = redGlobalMaster->puerto;
			nodoWorker->largoRutaArchivoReduccionLocal =
					redGlobalMaster->largoArchivoRedLocal;
			nodoWorker->rutaArchivoReduccionLocal = malloc(
					nodoWorker->largoRutaArchivoReduccionLocal);
			strcpy(nodoWorker->rutaArchivoReduccionLocal,
					redGlobalMaster->archivoRedLocal);

			list_add(redGlobalWorker.nodosAConectar, nodoWorker);
		}
		redGlobalWorker.cantidadNodos = list_size(
				redGlobalWorker.nodosAConectar);
	}

	if (socketWorkerEncargado == -1) {
		printf("envio desconexion del nodo a yama en el socket %d\n",
				socketYama);
		header.id = ERRORREDUCCION;
		header.tamanioPayload = 0;
		avisarAYamaRedGlobal(redGlobalWorker, header);
		//enviarPorSocket(socketYama, &header, sizeof(t_header));
		fallos++;
		log_error(masterLogger,
				"no se pudo conectar al worker encargado de la reduccion global.");
	} else {
		int tamanioMensaje, largoBuffer, desplazamiento = 0;
		void* bufferMensaje;
		void* buffer;
		buffer = serializarReduccionGlobalWorker(&redGlobalWorker,
				&largoBuffer);
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

		enviarPorSocket(socketWorkerEncargado, bufferMensaje, tamanioMensaje);
		free(buffer);
		free(bufferMensaje);

		if(respuestaTransformacion(socketWorkerEncargado) == REDUCCIONGLOBALOKWORKER){
			header.id = REDUCCIONGLOBALOKYAMA;
			printf("enviado a worker la reduccion global\n");
			log_info(masterLogger,
				"Se envia la operacion de reduccion global al worker %d",
				nodoEncargado);

		avisarAYamaRedGlobal(redGlobalWorker, header);

		reduccionGlobalRealizada++;
		}
		for (i = 0; i < list_size(redGlobalWorker.nodosAConectar); i++) {
			free(((t_datosNodoAEncargado*) list_get(
							redGlobalWorker.nodosAConectar, i))->ip);
			free(((t_datosNodoAEncargado*) list_get(
							redGlobalWorker.nodosAConectar, i))->rutaArchivoReduccionLocal);
		}
		list_destroy_and_destroy_elements(redGlobalWorker.nodosAConectar, free);
	}

	free(redGlobalWorker.archivoReductor);
	free(redGlobalWorker.rutaArchivoTemporal);
	t_fin = clock();
	tiempoTotalRedGlobal = (double) (t_fin - t_ini) / CLOCKS_PER_SEC * 1000;

}

int transformacionExistente(char* temporal){
	int i, posAux;
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
	headerResp.tamanioPayload = transformacion->largoArchivo + 1;
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

	enviarPorSocket(socketYama, buffer, desplazamiento);
	free(buffer);
}

void avisarAYamaRedLocal(t_infoReduccionesLocales reduccionWorker,
		t_header headerResp) {
	int desplazamiento;
	headerResp.tamanioPayload = reduccionWorker.largoRutaArchivoReductorLocal;
	void* buffer = malloc(sizeof(t_header) + headerResp.tamanioPayload);
	desplazamiento = 0;
	memcpy(buffer, &headerResp.id, sizeof(headerResp.id));
	desplazamiento += sizeof(headerResp.id);
	memcpy(buffer + desplazamiento, &headerResp.tamanioPayload,
			sizeof(headerResp.tamanioPayload));
	desplazamiento += sizeof(headerResp.tamanioPayload);
	memcpy(buffer + desplazamiento, reduccionWorker.rutaArchivoReductorLocal,
			headerResp.tamanioPayload);
	desplazamiento += headerResp.tamanioPayload;

	enviarPorSocket(socketYama, buffer, desplazamiento);
	free(buffer);
}

void avisarAYamaRedGlobal(t_infoReduccionGlobal redGlobalWorker,
		t_header headerResp) {
	int desplazamiento;
	headerResp.tamanioPayload = redGlobalWorker.largoRutaArchivoTemporal;
	void* buffer = malloc(sizeof(t_header) + headerResp.tamanioPayload);
	desplazamiento = 0;
	memcpy(buffer, &headerResp.id, sizeof(headerResp.id));
	desplazamiento += sizeof(headerResp.id);
	memcpy(buffer + desplazamiento, &headerResp.tamanioPayload,
			sizeof(headerResp.tamanioPayload));
	desplazamiento += sizeof(headerResp.tamanioPayload);
	memcpy(buffer + desplazamiento, redGlobalWorker.rutaArchivoTemporal,
			headerResp.tamanioPayload);
	desplazamiento += headerResp.tamanioPayload;

	enviarPorSocket(socketYama, buffer, desplazamiento);
	free(buffer);
}

void avisarAlmacenadoFinal() {
	int i, socketWorker, nodoEncargado;
	t_reduccionGlobalMaster* redGlobalMaster;
	t_header header;

	for (i = 0; i < list_size(listaRedGloblales); i++) {
		redGlobalMaster = list_get(listaRedGloblales, i);
		if (redGlobalMaster->encargado == 1) {
			socketWorker = conectarseAWorker(redGlobalMaster->puerto,
					redGlobalMaster->ip);
			nodoEncargado = redGlobalMaster->idNodo;
		}
	}

	if (socketWorker == -1) {
		printf("envio desconexion del nodo a yama en el socket %d\n",
				socketYama);
		header.id = ERRORALMACENADOFINAL;
		header.tamanioPayload = 0;
		enviarPorSocket(socketYama, &header, sizeof(t_header));
		fallos++;
		log_error(masterLogger,
				"no se pudo conectar al worker encargado de la reduccion global.");
	}

	header.id = ORDENALMACENADOFINAL;

	enviarPorSocket(socketWorker, &header, sizeof(t_header));

	printf(
			"se avisa al worker encargado (nodo %d) para almacenar el resultado del job\n",
			nodoEncargado);
	log_info(masterLogger,
			"Se avisa al worker encargado (nodo %d) para almacenar el resultado del job.",
			nodoEncargado);

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

	printf("%d\n", worker->largoArchivoTransformador);
	printf("%s\n", worker->archivoTransformador);

	pthread_mutex_lock(&mutexConexionWorker);
	int socketWorker = conectarseAWorker(transformacion->puerto,
			transformacion->ip);
	pthread_mutex_unlock(&mutexConexionWorker);
	if (socketWorker == -1) {
		printf("envio desconexion del nodo a yama en el socket %d\n",
				socketYama);
		//header.id = ERRORTRANSFORMACION;
		enviarFalloTransformacionAYama(transformacion,&header);
		log_error(masterLogger, "El worker %d no se encontro levantado.\n",
				transformacion->idNodo);
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

		enviarPorSocket(socketWorker, bufferMensaje, tamanioMensaje);
		free(buffer);
		free(bufferMensaje);
		printf("enviado a worker %d\n", transformacion->idNodo);
		log_info(masterLogger, "Enviado transformacion a worker %d",
				transformacion->idNodo);

		if (respuestaTransformacion(socketWorker) == TRANSFORMACIONOKWORKER) {
			printf("transformacion OK\n");
			pthread_mutex_lock(&mutexTotalTransformaciones);
			disminuirTransformacionesDeNodo(transformacion->idNodo);
			cantidadTransformacionesRealizadas++;
			list_add(archivosTranformacionOk, transformacion->archivoTransformacion);
			pthread_mutex_unlock(&mutexTotalTransformaciones);
			header.id = TRANSFORMACIONOKYAMA;
			avisarAYama(transformacion, header);
		}
		else{
			enviarFalloTransformacionAYama(transformacion, &header);
			log_error(masterLogger, "El worker %d no pudo terminar la ejecucion de la tarea de transformacion.\n",
							transformacion->idNodo);
			borrarTemporalesDeNodo(transformacion->idNodo);
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
	t_falloTransformacion fallo;
	fallo.largoRutaTemporal = transformacion->largoArchivo;
	fallo.rutaTemporalTransformacion = malloc(fallo.largoRutaTemporal);
	strcpy(fallo.rutaTemporalTransformacion,
			transformacion->archivoTransformacion);
	fallo.largoRutaArchivoAProcesar = strlen(archivoAprocesar);
	fallo.rutaArchivoAProcesar = malloc(fallo.largoRutaArchivoAProcesar);
	strcpy(fallo.rutaArchivoAProcesar, archivoAprocesar);

	tamanioBuffer = sizeof(t_header) + 2 * sizeof(uint32_t)
			+ fallo.largoRutaArchivoAProcesar + fallo.largoRutaTemporal;

	buffer = malloc(tamanioBuffer);

	header->id = ERRORTRANSFORMACION;
	header->tamanioPayload = tamanioBuffer - sizeof(t_header);

	memcpy(buffer, &header->id, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, &header->tamanioPayload, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, &fallo.largoRutaTemporal, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, fallo.rutaTemporalTransformacion,
			fallo.largoRutaTemporal);
	desplazamiento += fallo.largoRutaTemporal;
	memcpy(buffer + desplazamiento, &fallo.largoRutaArchivoAProcesar,
			sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, fallo.rutaArchivoAProcesar,
			fallo.largoRutaArchivoAProcesar);
	desplazamiento += fallo.largoRutaArchivoAProcesar;
	enviarPorSocket(socketYama, buffer, tamanioBuffer);
	pthread_mutex_lock(&mutexTotalFallos);
	fallos++;
	pthread_mutex_unlock(&mutexTotalFallos);
	free(fallo.rutaArchivoAProcesar);
	free(fallo.rutaTemporalTransformacion);
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
	printf("nodo> %d\n", nodo);
	for (i = 0; i < list_size(listaRedGloblales); i++) {
		if (nodosTransformacion[i].idNodo == nodo)
			nodosTransformacion[i].cantidadTransformaciones--;
	}
	transformacionesPendientes--;
}

int conectarseAWorker(int puerto, char* ip) {

	struct sockaddr_in direccionWorker;
	printf("puerto %d, ip %s\n", puerto, ip);

	direccionWorker.sin_family = AF_INET;
	direccionWorker.sin_port = htons(puerto);
	direccionWorker.sin_addr.s_addr = inet_addr(ip);
	//memset(&(direccionYama.sin_zero), '\0', 8);
	int socketWorker;

	socketWorker = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(socketWorker, (struct sockaddr *) &direccionWorker,
			sizeof(struct sockaddr)) != 0) {
		perror("fallo la conexion al worker");
		return -1;
	} else {
		printf("se conecto a un worker\n");
	}

	return socketWorker;
}

void* serializarTransformacionWorker(t_infoTransformacion* worker,
		int* largoBuffer) {
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

void* serializarReduccionLocalWorker(t_infoReduccionesLocales* redLocalWorker,
		int* largoBuffer) {
	int i, desplazamiento = 0, tamanioBuffer = 0;
	void* buffer;
	t_temporalesTransformacionWorker temporales;

	tamanioBuffer = 4 * sizeof(uint32_t) + redLocalWorker->largoArchivoReductor
			+ redLocalWorker->largoRutaArchivoReductorLocal;

	buffer = malloc(tamanioBuffer);

	memcpy(buffer + desplazamiento, &redLocalWorker->etapa, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(buffer + desplazamiento,
			&redLocalWorker->largoRutaArchivoReductorLocal, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, redLocalWorker->rutaArchivoReductorLocal,
			redLocalWorker->largoRutaArchivoReductorLocal);
	desplazamiento += redLocalWorker->largoRutaArchivoReductorLocal;

	memcpy(buffer + desplazamiento, &redLocalWorker->largoArchivoReductor,
			sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, redLocalWorker->archivoReductor,
			redLocalWorker->largoArchivoReductor);
	desplazamiento += redLocalWorker->largoArchivoReductor;

	memcpy(buffer + desplazamiento, &redLocalWorker->cantidadTransformaciones,
			sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	for (i = 0; i < list_size(redLocalWorker->temporalesTranformacion); i++) {
		temporales = *(t_temporalesTransformacionWorker*) (list_get(
				redLocalWorker->temporalesTranformacion, i));
		tamanioBuffer += temporales.largoRutaTemporalTransformacion
				+ sizeof(uint32_t);
		buffer = realloc(buffer, tamanioBuffer);

		memcpy(buffer + desplazamiento,
				&temporales.largoRutaTemporalTransformacion, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(buffer + desplazamiento, temporales.rutaTemporalTransformacion,
				temporales.largoRutaTemporalTransformacion);
		desplazamiento += temporales.largoRutaTemporalTransformacion;
	}

	*largoBuffer = tamanioBuffer;
	return buffer;
}

void* serializarReduccionGlobalWorker(t_infoReduccionGlobal* redGlobalWorker,
		int* largoBuffer) {
	int i, desplazamiento = 0, tamanioBuffer = 0;
	void* buffer;
	t_datosNodoAEncargado nodos;
	tamanioBuffer = 4 * sizeof(uint32_t) + redGlobalWorker->largoArchivoReductor
			+ redGlobalWorker->largoRutaArchivoTemporal;
	buffer = malloc(tamanioBuffer);

	memcpy(buffer + desplazamiento, &redGlobalWorker->etapa, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, &redGlobalWorker->largoRutaArchivoTemporal,
			sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, redGlobalWorker->rutaArchivoTemporal,
			redGlobalWorker->largoRutaArchivoTemporal);
	desplazamiento += redGlobalWorker->largoRutaArchivoTemporal;

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
		nodos = *(t_datosNodoAEncargado*) (list_get(
				redGlobalWorker->nodosAConectar, i));
		tamanioBuffer += nodos.largoRutaArchivoReduccionLocal + nodos.largoIp
				+ 3 * sizeof(uint32_t);
		buffer = realloc(buffer, tamanioBuffer);
		memcpy(buffer + desplazamiento, &nodos.puerto, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);

		memcpy(buffer + desplazamiento, &nodos.largoIp, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(buffer + desplazamiento, nodos.ip, nodos.largoIp);
		desplazamiento += nodos.largoIp;

		memcpy(buffer + desplazamiento, &nodos.largoRutaArchivoReduccionLocal,
				sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(buffer + desplazamiento, nodos.rutaArchivoReduccionLocal,
				nodos.largoRutaArchivoReduccionLocal);
		desplazamiento += nodos.largoRutaArchivoReduccionLocal;
	}

	*largoBuffer = tamanioBuffer;
	return buffer;
}

int respuestaTransformacion(int socketWorker) {

	char* buffer = malloc(sizeof(int));
	int respuesta;
	recibirPorSocket(socketWorker, &respuesta, sizeof(int));
	free(buffer);
	return respuesta;
}

void metricas(double tiempo) {
	double promedioTransformaciones = tiempoTotalTransformaciones;
		//	/ (double) list_size(listaTransformaciones);
	double promedioReducciones = tiempoTotalRedLocales;
		//	/ (double) list_size(listaRedGloblales);
	printf("\n--metricas--\n\n");
	printf("tiempo de ejecucion del job: %.16g milisegundos\n", tiempo * 1000);
	printf("tiempo promedio de ejecucion de etapa de transformacion: %.16g milisegundos\n",
			promedioTransformaciones);
	printf("tiempo promedio de ejecucion de etapa de reduccion local: %.16g milisegundos\n",
			promedioReducciones);
	printf("tiempo promedio de ejecucion de etapa de reduccion global: %.16g milisegundos\n",
			tiempoTotalRedGlobal);
	printf("total de transformaciones realizadas: %d\n",
			cantidadTransformacionesRealizadas);
	printf("total de reducciones locales realizadas: %d\n",
			cantidadReduccionesLocalesRealizadas);
	printf("total de reducciones globales realizadas: %d\n",
			reduccionGlobalRealizada);
	printf("cantidad maxima de tareas de transformacion en paralelo: %d\n",
			maximoTareasCorriendoTransformacion);
	printf("cantidad maxima de tareas de reduccion local en paralelo: %d\n",
				maximoTareasCorriendoRedLocal);
	printf("cantidad de fallos obtenidos: %d\n", fallos);
}

int devolverTamanioArchivo(char* archivo) {
	printf("%s\n", archivo);
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
	return tam;
}

char* obtenerContenidoArchivo(char* archivo) {
	char* buffer;
	int file = open(archivo, O_RDWR);
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
	buffer = (char*) malloc(tam * sizeof(char) + 1);
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
