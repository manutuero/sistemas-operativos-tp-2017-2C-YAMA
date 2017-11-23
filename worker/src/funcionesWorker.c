#include "funcionesWorker.h"

void crearLogger() {
	char *pathLogger = string_new();
	char cwd[1024];
	string_append(&pathLogger, getcwd(cwd, sizeof(cwd)));
	string_append(&pathLogger, "/workerLogs.log");

	char *logWorkerFileName = strdup("workerLogs.log");
	workerLogger = log_create(pathLogger, logWorkerFileName, false, LOG_LEVEL_INFO);
	free(logWorkerFileName);

	logWorkerFileName = NULL;
}
void cargarArchivoConfiguracion(char*nombreArchivo) {
	log_info(workerLogger, "Cargando archivo de configuracion del FileSystem.");
	char cwd[1024]; // Variable donde voy a guardar el path absoluto
	char * pathArchConfig = string_from_format("%s/%s",getcwd(cwd, sizeof(cwd)), nombreArchivo); // String que va a tener el path absoluto para pasarle al config_create
	t_config *config = config_create(pathArchConfig);

	if(!config) {
	  perror("[ERROR]: No se pudo cargar el archivo de configuracion.");
	  exit(EXIT_FAILURE);
	 }

	log_info(workerLogger,
			"El directorio sobre el que se esta trabajando es %s.",
			pathArchConfig);
	if (config_has_property(config, "PUERTO_FILESYSTEM")) {
		PUERTO_FILESYSTEM = config_get_int_value(config, "PUERTO_FILESYSTEM");
	}
	if (config_has_property(config, "IP_FILESYSTEM")) {
		IP_FILESYSTEM = config_get_string_value(config, "IP_FILESYSTEM");
	}
	if (config_has_property(config, "ID_NODO")) {
		ID_NODO = config_get_string_value(config, "ID_NODO");
	}
	if (config_has_property(config, "PUERTO_WORKER")) {
		PUERTO_WORKER = config_get_int_value(config, "PUERTO_WORKER");
	}
	if (config_has_property(config, "PUERTO_DATANODE")) {
		PUERTO_DATANODE = config_get_int_value(config, "PUERTO_DATANODE");
		}
	if (config_has_property(config, "RUTA_DATABIN")) {
		RUTA_DATABIN = config_get_string_value(config, "RUTA_DATABIN");
	}

	printf("\nIP Filesystem: %s\n", IP_FILESYSTEM);
	printf("\nPuerto Filesystem: %d\n", PUERTO_FILESYSTEM);
	printf("\nID Nodo %s\n", ID_NODO);
	printf("\nPuerto Worker %d\n", PUERTO_WORKER);
	printf("\nPuerto Datanode %d\n",PUERTO_DATANODE);
	printf("\nRuta Data.bin %s\n", RUTA_DATABIN);
	log_info(workerLogger, "Archivo de configuracion cargado exitosamente");

	//config_destroy(config);  //si se descomenta esta linea, se reduce la cantidad de memory leaks
}


int recibirArchivo(int cliente) {

	struct stat buf;
	if (stat("/home/utnso/Escritorio/archivoSalida", &buf) == 0) {
		remove("/home/utnso/Escritorio/archivoSalida");
	}

	//FILE* file;
	//file = fopen("/home/utnso/Escritorio/archivoSalida","wb");
	int file = open("/home/utnso/Escritorio/archivoSalida",
			O_WRONLY | O_CREAT | O_TRUNC);

	void *buffer;
	t_header header;

	int bytesRecibidos = recibirHeader(cliente, &header);
	//memset(file,0,header.tamanio-sizeof(uint32_t));

	buffer = malloc(header.tamanioPayload);
	//memset(buffer,0,header.tamanio);

	bytesRecibidos = recibirPorSocket(cliente, buffer, header.tamanioPayload);
	if (bytesRecibidos > 0) {

		t_archivo* archivo;

		//Se podria hacer RecibirPaquete, dependiendo si la deserializacion va a utils o no.
		archivo = (t_archivo*) deserializarArchivo(buffer);
		//deserializarArchivo(buffer,&archivo);

		printf("Al worker le llego un archivo de %d bytes: \n%s\n",
				archivo->tamanio, archivo->contenido);

		printf("Bytes del archivo: %d\n", archivo->tamanio);

		//fwrite((char*)archivo->contenido, archivo->tamanio,1,file);
		//fclose(file);
		write(file, archivo->contenido, archivo->tamanio);
		close(file);

		free(archivo->contenido);
		free(archivo);
		free(buffer);
		return bytesRecibidos;
	} else {
		perror("Error al recibir el archivo");
		return 0;
	}
}

t_archivo* deserializarArchivo(void *buffer) {
	t_archivo *miArchivo = malloc(sizeof(t_archivo));

	int desplazamiento = 0;
	memcpy(&miArchivo->tamanio, buffer, sizeof(miArchivo->tamanio));
	desplazamiento += sizeof(miArchivo->tamanio);

	miArchivo->contenido = malloc(miArchivo->tamanio);
	memcpy(miArchivo->contenido, buffer + desplazamiento, miArchivo->tamanio);
	//miArchivo->contenido[miArchivo->tamanio]='\0';

	//free(miArchivo->contenido);

	return miArchivo;
}

void abrirDatabin() {
	filePointer = fopen(RUTA_DATABIN, "r+");
	// Valida que el archivo exista. Caso contrario lanza error.
	if (!filePointer) {
		perror("El archivo 'data.bin' no existe en la ruta especificada.");
		exit(EXIT_FAILURE);
	}
	fileDescriptor = fileno(filePointer);
}
void cerrarDatabin() {
	fclose(filePointer);
}


char* getBloque(int numero) {
	size_t bytesALeer = UN_BLOQUE, tamanioArchivo;
	off_t desplazamiento = numero * UN_BLOQUE;
	char *regionDeMapeo, *data = malloc(bytesALeer+1);

	// Estructura que contiene informacion del estado de archivos y dispositivos.
	struct stat st;
	stat(RUTA_DATABIN, &st);
	tamanioArchivo = st.st_size;

	if (desplazamiento >= tamanioArchivo) {
		perror("El desplazamiento sobrepaso el fin de archivo (EOF).");
		exit(EXIT_FAILURE);
	}

	if (desplazamiento + bytesALeer > tamanioArchivo)
		bytesALeer = tamanioArchivo - desplazamiento;
	/* No puedo mostrar bytes pasando el EOF */

	regionDeMapeo = mmap(NULL, bytesALeer,
	PROT_READ, MAP_SHARED, fileDescriptor, desplazamiento);

	if (regionDeMapeo == MAP_FAILED) {
		perror("No se pudo reservar la region de mapeo.");
		exit(EXIT_FAILURE);
	}

	strncpy(data, regionDeMapeo, bytesALeer);
	munmap(regionDeMapeo, bytesALeer); // Libero la region de mapeo solicitada.
	return data;
	free(regionDeMapeo);
}


void realizarTransformacion(t_infoTransformacion infoTransformacion){
	int respuesta;
	char* rutaArchTransformador;
	FILE*archivoTemp;
	rutaArchTransformador=guardarArchScript(infoTransformacion.archTransformador);
	archivoTemp=fopen(infoTransformacion.nombreArchTemp,"r+");
	char* bloque;
	char*data;
	bloque=getBloque(infoTransformacion.numBloque);
	data=memcpy(data,bloque,infoTransformacion.bytesOcupados);
	char* lineaAEjecutar=string_new();
	string_append(&data,"\0");
	string_append_with_format(&lineaAEjecutar,"echo -e %s | ./%s | sort > %s",data,rutaArchTransformador,infoTransformacion.nombreArchTemp);

	respuesta =system(lineaAEjecutar);

	//guardarArchTempEnDestino(archivoTemp,destinoArchTemp);
	//registrarOperacionEnLogs(repuesta)
	fclose(archivoTemp);
}

char *guardarArchScript(char*contenidoArchivoScript) {
	char*rutaArchScritp=string_new();
	string_append(&rutaArchScritp,"home/utnso/scriptsGuardados/archTransformador");
	FILE*arch=fopen(rutaArchScritp,"w");
	txt_write_in_file(arch,contenidoArchivoScript);
	fclose(arch);
	return rutaArchScritp;
}

char* armarNombreConPathTemp(char* nombreTemp){
	char * nombreArchTemp= string_new();
	string_append(&nombreArchTemp,pathTemporales);
	string_append(&nombreArchTemp,nombreTemp);
	return nombreArchTemp;
}

t_infoTransformacion deserializarInfoTransformacion(void* buffer) {
	t_infoTransformacion infoTransformacion;
	uint32_t desplazamiento=0,bytesACopiar;

	bytesACopiar=sizeof(uint32_t);
	memcpy(&infoTransformacion.numBloque,buffer+desplazamiento,bytesACopiar);
	desplazamiento+=bytesACopiar;

	bytesACopiar=sizeof(uint32_t);
	memcpy(&infoTransformacion.bytesOcupados,buffer+desplazamiento,bytesACopiar);
	desplazamiento+=bytesACopiar;

	bytesACopiar=sizeof(uint32_t);
	memcpy(&infoTransformacion.largoNombreArchTemp,buffer+desplazamiento,bytesACopiar);
	desplazamiento+=bytesACopiar;

	bytesACopiar=infoTransformacion.largoNombreArchTemp;
	memcpy(&infoTransformacion.nombreArchTemp,buffer+desplazamiento,bytesACopiar);
	desplazamiento+=bytesACopiar;

	bytesACopiar=sizeof(uint32_t);
	memcpy(&infoTransformacion.largoArchTransformador,buffer+desplazamiento,bytesACopiar);
	desplazamiento+=bytesACopiar;

	bytesACopiar=infoTransformacion.largoArchTransformador;
	memcpy(&infoTransformacion.archTransformador,buffer+desplazamiento,bytesACopiar);
	desplazamiento+=bytesACopiar;

	return infoTransformacion;
}

void realizarReduccionLocal(t_infoReduccionLocal* infoReduccionLocal) {
	//verificarExistenciaPathTemp(pathTemporales);
	char*rutaArchReductor;
	rutaArchReductor = guardarArchScript(infoReduccionLocal->archReductor);
	FILE* archivoReduccionLocal;
	archivoReduccionLocal = fopen(infoReduccionLocal->rutaArchReducidoLocal,
			"r+");
	char*lineaAEjecutar = string_new();
	int i;
	int resultado;
	FILE* archivo;
	char linea[LARGO_MAX_LINEA];
	for (i = 0; i < infoReduccionLocal->cantidadTransformaciones; i++) {
		archivo = fopen(list_get(infoReduccionLocal->archTemporales, i), "r+");
		while (fgets(linea, LARGO_MAX_LINEA, archivo) != NULL) {
			txt_write_in_file(archivoReduccionLocal, linea);
		}
		fclose(archivo);
	}

	string_append_with_format(&lineaAEjecutar, "echo -e %s | sort | ./%s > %s",
			archivoReduccionLocal, rutaArchReductor,
			infoReduccionLocal->rutaArchReducidoLocal);
	resultado = system(lineaAEjecutar);
}

t_infoReduccionLocal* deserializarInfoReduccionLocal(void*buffer) {
	t_infoReduccionLocal* infoReduccionLocal = malloc(
			sizeof(t_infoReduccionLocal));
	uint32_t bytesACopiar, desplazamiento = 0, i = 0;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoReduccionLocal->largoRutaArchReducidoLocal,
			buffer + desplazamiento, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = infoReduccionLocal->largoRutaArchReducidoLocal;
	infoReduccionLocal->rutaArchReducidoLocal = malloc(
			infoReduccionLocal->largoRutaArchReducidoLocal);
	memcpy(infoReduccionLocal->rutaArchReducidoLocal, buffer + desplazamiento,
			bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoReduccionLocal->largoArchivoReductor, buffer + desplazamiento,
			bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = infoReduccionLocal->largoArchivoReductor;
	infoReduccionLocal->archReductor = malloc(
			infoReduccionLocal->largoArchivoReductor);
	memcpy(infoReduccionLocal->archReductor, buffer + desplazamiento,
			bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoReduccionLocal->cantidadTransformaciones,
			buffer + desplazamiento, bytesACopiar);
	desplazamiento += bytesACopiar;

	infoReduccionLocal->archTemporales = list_create();
	for (i = 0; i < infoReduccionLocal->cantidadTransformaciones; i++) {
		t_temporalesTransformacionWorker* temporal = malloc(
				sizeof(t_temporalesTransformacionWorker));
		bytesACopiar = sizeof(uint32_t);
		memcpy(&temporal->largoRutaTemporalTransformacion,
				buffer + desplazamiento, bytesACopiar);
		desplazamiento += bytesACopiar;

		bytesACopiar = sizeof(temporal->largoRutaTemporalTransformacion);
		temporal->rutaTemporalTransformacion = malloc(
				temporal->largoRutaTemporalTransformacion);
		memcpy(temporal->rutaTemporalTransformacion, buffer + desplazamiento,
				bytesACopiar);
		desplazamiento += bytesACopiar;

		list_add(infoReduccionLocal->archTemporales,
				temporal->rutaTemporalTransformacion);
	}

	return infoReduccionLocal;
}

char *guardarArchScript(char*contenidoArchivoScript) {
	char*rutaArchScritp = string_new();
	string_append(&rutaArchScritp,
			"/home/utnso/thePonchos/scriptsGuardados/archScript");
	FILE*arch = fopen(rutaArchScritp, "w");
	txt_write_in_file(arch, contenidoArchivoScript);
	fclose(arch);
	return rutaArchScritp;
}
