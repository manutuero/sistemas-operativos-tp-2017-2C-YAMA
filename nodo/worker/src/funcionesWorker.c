#include "funcionesWorker.h"

void crearLogger() {
	char *pathLogger = string_new();
	char cwd[1024];
	string_append(&pathLogger, getcwd(cwd, sizeof(cwd)));
	string_append(&pathLogger, "/logs/workerLogs.log");

	char *logWorkerFileName = strdup("workerLogs.log");
	workerLogger = log_create(pathLogger, logWorkerFileName, false,
			LOG_LEVEL_INFO);
	free(logWorkerFileName);

	logWorkerFileName = NULL;
}
void cargarArchivoConfiguracion(char*nombreArchivo) {
	log_info(workerLogger, "Cargando archivo de configuracion del FileSystem.");
	char cwd[1024]; // Variable donde voy a guardar el path absoluto
	char * pathArchConfig = string_from_format("%s/%s",
			getcwd(cwd, sizeof(cwd)), nombreArchivo); // String que va a tener el path absoluto para pasarle al config_create
	t_config *config = config_create(pathArchConfig);

	if (!config) {
		perror("[ERROR]: No se pudo cargar el archivo de configuracion.");
		exit(EXIT_FAILURE);
	}

	log_info(workerLogger,
			"El directorio sobre el que se esta trabajando es %s.",
			pathArchConfig);
	if (config_has_property(config, "PUERTO_FS_WORKER")) {
		PUERTO_FILESYSTEM = config_get_int_value(config, "PUERTO_FS_WORKER");
	}
	if (config_has_property(config, "IP_FILESYSTEM")) {
		IP_FILESYSTEM = config_get_string_value(config, "IP_FILESYSTEM");
	}
	if (config_has_property(config, "ID_NODO")) {
		ID_NODO = config_get_int_value(config, "ID_NODO");
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
	if (config_has_property(config, "RUTA_TEMPORALES")) {
		RUTA_TEMPORALES = config_get_string_value(config, "RUTA_TEMPORALES");
	}

	printf("\nIP Filesystem: %s\n", IP_FILESYSTEM);
	printf("\nPuerto Filesystem: %d\n", PUERTO_FILESYSTEM);
	printf("\nID Nodo %d\n", ID_NODO);
	printf("\nPuerto Worker %d\n", PUERTO_WORKER);
	printf("\nPuerto Datanode %d\n", PUERTO_DATANODE);
	printf("\nRuta Data.bin %s\n", RUTA_DATABIN);
	log_info(workerLogger, "Archivo de configuracion cargado exitosamente");

	//config_destroy(config);  //si se descomenta esta linea, se reduce la cantidad de memory leaks
}

t_archivo* deserializarArchivo(void *buffer) {
	t_archivo *miArchivo = malloc(sizeof(t_archivo));

	int desplazamiento = 0;
	memcpy(&miArchivo->tamanio, buffer, sizeof(miArchivo->tamanio));
	desplazamiento += sizeof(miArchivo->tamanio);

	miArchivo->contenido = malloc(miArchivo->tamanio);
	memcpy(miArchivo->contenido, buffer + desplazamiento, miArchivo->tamanio);

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
	char *regionDeMapeo, *data = malloc(bytesALeer);

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
	//free(regionDeMapeo);
	return data;

}

void realizarTransformacion(t_infoTransformacion* infoTransformacion,
		int socketMaster) {
	int respuesta;
	char*rutaGuardadoTemp;
	char* rutaArchTransformador;
	char*rutaDataBloque = string_new();
	string_append(&rutaDataBloque, "/home/utnso/thePonchos");
	string_append(&rutaDataBloque, infoTransformacion->nombreArchTemp);
	string_append(&rutaDataBloque, "D");
	FILE* dataBloque = fopen(rutaDataBloque, "w+");
	FILE*archivoTemp;
	rutaArchTransformador = guardarArchScript(
			infoTransformacion->archTransformador,
			infoTransformacion->nombreArchTemp);
	char* bloque;
	char*data = malloc(infoTransformacion->bytesOcupados);
	bloque = getBloque(infoTransformacion->numBloque);
	data = memcpy(data, bloque, infoTransformacion->bytesOcupados);
	txt_write_in_file(dataBloque, data);
	char* lineaAEjecutar = string_new();
	rutaGuardadoTemp = armarRutaGuardadoTemp(
			infoTransformacion->nombreArchTemp);
	archivoTemp = fopen(rutaGuardadoTemp, "w+");
	string_append_with_format(&lineaAEjecutar, "cat %s | %s | sort > %s",
			rutaDataBloque, rutaArchTransformador, rutaGuardadoTemp);

	free(bloque);
	free(data);

	respuesta = system(lineaAEjecutar);

	fclose(dataBloque);
	fclose(archivoTemp);
	remove(rutaArchTransformador);
	remove(rutaDataBloque);
	free(rutaDataBloque);
	free(rutaArchTransformador);

	if (respuesta != -1) {
		wait(&respuesta);
		notificarAMaster(TRANSFORMACION_OK, socketMaster);
		log_info(workerLogger, "Transformacion del bloque %d realizada",
				infoTransformacion->numBloque);
		log_info(workerLogger,"Se genera archivo temporal %s:",infoTransformacion->nombreArchTemp);
		free(lineaAEjecutar);
	} else {
		notificarAMaster(ERROR_TRANSFORMACION,socketMaster);
		log_info(workerLogger,
				"No se pudo realizar la transformacion en el bloque",
				infoTransformacion->numBloque);
	}

	free(infoTransformacion->archTransformador);
	free(infoTransformacion->nombreArchTemp);
	free(infoTransformacion);

}

char* armarRutaGuardadoTemp(char*nombreTemp) {
	char*rutaGuardadoTemp = string_new();
	string_append(&rutaGuardadoTemp, RUTA_TEMPORALES);
	string_append(&rutaGuardadoTemp, nombreTemp);
	return rutaGuardadoTemp;

}

char *guardarArchScript(char*contenidoArchivoScript, char* nombreArchTemp) {
	char*rutaArchScritp = string_new();
	char modo[] = "0777";
	int permiso;
	char* nombreSolo = string_new();
	nombreSolo = string_substring_from(nombreArchTemp, 4);
	string_append(&rutaArchScritp, "/home/utnso/thePonchos/tmp/scripts");
	string_append(&rutaArchScritp, nombreSolo);
	FILE*arch = fopen(rutaArchScritp, "w+");
	permiso = strtol(modo, 0, 8);
	chmod(rutaArchScritp, permiso);
	txt_write_in_file(arch, contenidoArchivoScript);
	fclose(arch);
	free(nombreSolo);

	return rutaArchScritp;
}

void realizarReduccionLocal(t_infoReduccionLocal* infoReduccionLocal,
	int socketMaster) {
	printf("Inicio reduccion local,job: %s\n", infoReduccionLocal->rutaArchReducidoLocal);
	char*rutaReducidoLocal;
	char*rutaArchReductor;
	rutaArchReductor = guardarArchScript(infoReduccionLocal->archReductor,
			infoReduccionLocal->rutaArchReducidoLocal);
	char*rutaArchConcat;
	rutaArchConcat = armarRutaGuardadoTemp(
			infoReduccionLocal->rutaArchReducidoLocal);
	string_append(&rutaArchConcat, "C");
	FILE*archivoConcat = fopen(rutaArchConcat, "w+");
	FILE* archivoReduccionLocal;
	rutaReducidoLocal = armarRutaGuardadoTemp(
			infoReduccionLocal->rutaArchReducidoLocal);
	archivoReduccionLocal = fopen(rutaReducidoLocal, "w+");
	char*lineaAEjecutar = string_new();
	int i;
	int resultado;
	FILE* archivo;
	char*linea;
	for (i = 0; i < infoReduccionLocal->cantidadTransformaciones; i++) {
		archivo = fopen(
				armarRutaGuardadoTemp(
						(char*) list_get(infoReduccionLocal->archTemporales,
								i)), "r+");
		while (!feof(archivo)) {
			linea = malloc(LARGO_MAX_LINEA);
			proximoRegistro(archivo, linea);
			txt_write_in_file(archivoConcat, linea);
			free(linea);
		}
		fclose(archivo);
	}

	string_append_with_format(&lineaAEjecutar, "cat %s | sort | %s > %s",
			rutaArchConcat, rutaArchReductor, rutaReducidoLocal);

	resultado = system(lineaAEjecutar);

	fclose(archivoConcat);
	remove(rutaArchConcat);
	remove(rutaArchReductor);
	fclose(archivoReduccionLocal);

	if (resultado != -1) {
		wait(&resultado);
		notificarAMaster(REDUCCION_LOCAL_OK, socketMaster);
		log_info(workerLogger, "Reduccion local realizada,archivo %s generado",
				infoReduccionLocal->rutaArchReducidoLocal);
		free(lineaAEjecutar);
	} else {
		notificarAMaster(ERROR_REDUCCION, socketMaster);
		log_info(workerLogger, "Error en reduccion local,no se pudo generar el temporal %s:",
				infoReduccionLocal->rutaArchReducidoLocal);
	}
	free(infoReduccionLocal->archReductor);
	free(infoReduccionLocal->rutaArchReducidoLocal);
	list_destroy_and_destroy_elements(infoReduccionLocal->archTemporales, free);
	free(infoReduccionLocal);

	free(rutaArchConcat);
	free(rutaArchReductor);
	free(rutaReducidoLocal);
}

int proximoRegistro(FILE *datos, char *registro) {
	int largo = 0;
	char caracter = fgetc(datos);
	// Recorro el stream de datos.
	while (caracter != '\n' && !feof(datos)) {
		registro[largo++] = caracter;

		// Los registros son de maximo 1 MB. Considero 1 caracter por el '\n'.
		if (largo > UN_MEGABYTE) {
			puts("[ERROR]: El registro a escribir es mayor a 1 MB.");
			exit(EXIT_FAILURE);
		}
		caracter = fgetc(datos);
	}
	if (caracter == '\n') {
		registro[largo++] = caracter;
	}
	return largo;
}

void realizarReduccionGlobal(t_infoReduccionGlobal* infoReduccionGlobal,
		int socketMaster) {
	printf("Inicio reduccion Global,job: %s\n",infoReduccionGlobal->rutaArchivoTemporalFinal);
	char* lineaAEjecutar = string_new();
	char* contenidoArchRecibido = string_new();
	int i, socketDePedido, encontrado;
	int resultado;
	FILE* archivoReduccionGlobal;
	char*rutaArchReducidoFinal;
	char*rutaArchAAparear;
	char*rutaArchApareado;
	char*rutaArchReductor;
	rutaArchReductor = guardarArchScript(infoReduccionGlobal->archivoReductor,
			infoReduccionGlobal->rutaArchivoTemporalFinal);
	rutaArchAAparear = armarRutaGuardadoTemp(
			infoReduccionGlobal->rutaArchivoTemporalFinal);
	string_append(&rutaArchAAparear, "A");

	rutaArchApareado = armarRutaGuardadoTemp(
			infoReduccionGlobal->rutaArchivoTemporalFinal);
	string_append(&rutaArchApareado, "AP");
	FILE*archivoApareado = fopen(rutaArchApareado, "w+");
	if (!archivoApareado) {
		fprintf(stderr, "no se pudo crear el archivo\n");
	}
	rutaArchReducidoFinal = armarRutaGuardadoTemp(
			infoReduccionGlobal->rutaArchivoTemporalFinal);
	archivoReduccionGlobal = fopen(rutaArchReducidoFinal, "w+");
	char*rutaTempRecibido;
	rutaTempRecibido = armarRutaGuardadoTemp(
			infoReduccionGlobal->rutaArchivoTemporalFinal);
	string_append(&rutaTempRecibido, "recibido");
	char*rutaArchLocal;
	rutaArchLocal = armarRutaGuardadoTemp(
			infoReduccionGlobal->rutaArchivoLocal);
	FILE*archLocal = fopen(rutaArchLocal, "r");
	copiarContenidoDeArchivo(archivoApareado, archLocal);

	for (i = 0; i < infoReduccionGlobal->cantidadNodos; i++) {
		rewind(archivoApareado);
		t_datosNodoAEncargado infoArchivo = *(t_datosNodoAEncargado*) list_get(
				infoReduccionGlobal->nodosAConectar, i);
		rutaTempRecibido = armarRutaGuardadoTemp(
				infoArchivo.rutaArchivoReduccionLocal);
		FILE* archivoRecibido = fopen(rutaTempRecibido, "w+");
		socketDePedido = solicitarArchivoAWorker(infoArchivo.ip,
				infoArchivo.puerto, infoArchivo.rutaArchivoReduccionLocal,
				infoArchivo.largoRutaArchivoReduccionLocal);
		if (socketDePedido != -1) {
			contenidoArchRecibido = (char*) recibirArchivoTemp(socketDePedido,
					&encontrado);

			if (encontrado != ERROR_ARCHIVO_NO_ENCONTRADO) {
				txt_write_in_file(archivoRecibido, contenidoArchRecibido);
				rewind(archivoRecibido);
				free(contenidoArchRecibido);
				aparearArchivos(rutaArchAAparear, archivoRecibido,
						archivoApareado);

			} else {
				notificarAMaster(ERROR_REDUCCION_GLOBAL, socketMaster);
				log_info(workerLogger,"Error Reduccion global,no se encontro archivo en otro worker");
				break;
			}
		} else {
			notificarAMaster(ERROR_REDUCCION_GLOBAL, socketMaster);
			log_info(workerLogger,"Error Reduccio global,error en conexion a otro worker");
		}
		fclose(archivoRecibido);
		remove(rutaTempRecibido);
		free(rutaTempRecibido);
	}

	if (i == infoReduccionGlobal->cantidadNodos) {

		string_append_with_format(&lineaAEjecutar, "cat %s | %s > %s",
				rutaArchApareado, rutaArchReductor, rutaArchReducidoFinal);

		resultado = system(lineaAEjecutar);

		if (resultado != -1) {
			wait(&resultado);
			notificarAMaster(REDUCCION_GLOBAL_OK, socketMaster);
			log_info(workerLogger,
					"Reduccion Global realizada, archivo %s generado",
					infoReduccionGlobal->rutaArchivoTemporalFinal);
		} else {
			notificarAMaster(ERROR_REDUCCION, socketMaster);
			log_info(workerLogger, "Error en reduccion global");
		}

		fclose(archivoApareado);
		fclose(archivoReduccionGlobal);
		remove(rutaArchAAparear);
		remove(rutaArchApareado);
		remove(rutaArchReductor);
		free(rutaArchAAparear);
		free(rutaArchApareado);
		free(rutaArchLocal);
		free(rutaArchReducidoFinal);
		free(rutaArchReductor);
		free(lineaAEjecutar);
	} else {
		notificarAMaster(ERROR_REDUCCION, socketMaster);
		log_info(workerLogger, "Error en reduccion global,no se genero %s",
				infoReduccionGlobal->rutaArchivoTemporalFinal);
	}

}

void aparearArchivos(char* rutaArchAAparear, FILE* archivoRecibido,
		FILE* archivoApareado) {
	t_regArch regArch1 = malloc(LARGO_MAX_LINEA);
	t_regArch regArch2 = malloc(LARGO_MAX_LINEA);
	FILE* archAAparear = fopen(rutaArchAAparear, "w+");
	bool finArch1 = false;
	bool finArch2 = false;
	copiarContenidoDeArchivo(archAAparear, archivoApareado);
	rewind(archAAparear);
	rewind(archivoApareado);

	leerRegArchivo(archAAparear, regArch1, &finArch1);
	leerRegArchivo(archivoRecibido, regArch2, &finArch2);
	while (!feof(archAAparear) && !feof(archivoRecibido)) {
		if (strcmp(regArch1, regArch2) < 0) {
			txt_write_in_file(archivoApareado, regArch1);
			leerRegArchivo(archAAparear, regArch1, &finArch1);
		} else {
			txt_write_in_file(archivoApareado, regArch2);
			leerRegArchivo(archivoRecibido, regArch2, &finArch2);
		}
	}
	if (!feof(archAAparear)) {
		while (!feof(archAAparear)) {
			txt_write_in_file(archivoApareado, regArch1);
			leerRegArchivo(archAAparear, regArch1, &finArch1);
		}
	} else {
		while (!feof(archivoRecibido)) {
			txt_write_in_file(archivoApareado, regArch2);
			leerRegArchivo(archivoRecibido, regArch2, &finArch2);
		}
	}
	fclose(archAAparear);
	free(regArch1);
	free(regArch2);
}

void leerRegArchivo(FILE* arch, t_regArch regArch, bool* fin) {
	if (!(feof(arch))) {
		fgets(regArch, LARGO_MAX_LINEA, arch);
		*fin = false;
	} else {
		*fin = true;
	}
}

void copiarContenidoDeArchivo(FILE* archivoCopiado, FILE* archivoACopiar) {
	t_regArch regArchivoACopiar;
	while (!(feof(archivoACopiar))) {
		regArchivoACopiar = malloc(LARGO_MAX_LINEA);
		fgets(regArchivoACopiar, LARGO_MAX_LINEA, archivoACopiar);
		txt_write_in_file(archivoCopiado, regArchivoACopiar);
		free(regArchivoACopiar);
	}

}

void* recibirArchivoTemp(int socketDePedido, int* encontrado) {
	void*buffer;
	t_header header;
	recibirHeader(socketDePedido, &header);
	if (header.id == RECIBIR_ARCH_TEMP) {
		buffer = malloc(header.tamanioPayload);
		recibirPorSocket(socketDePedido, buffer, header.tamanioPayload);
		return buffer;
		//free(buffer);
	} else if (header.id == ERROR_ARCHIVO_NO_ENCONTRADO) {
		*encontrado = ERROR_ARCHIVO_NO_ENCONTRADO;
	}
	return "";
}

char* deserializarRecepcionArchivoTemp(void* buffer) {

	char* archTemporal;
	int desplazamiento = 0;
	int bytesACopiar;
	int largoArchTemporal;
	bytesACopiar = sizeof(uint32_t);
	memcpy(&largoArchTemporal, buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += bytesACopiar;

	bytesACopiar = largoArchTemporal;
	archTemporal = malloc(largoArchTemporal);
	memcpy(archTemporal, buffer + desplazamiento, largoArchTemporal);
	desplazamiento += bytesACopiar;
	return archTemporal;
}

int solicitarArchivoAWorker(char*ip, int puerto, char*nombreArchivoTemp,
		int largoNombreArchTemp) {
	void* buffer;
	void* bufferMensaje;
	t_header header;
	int largoBuffer, tamanioMensaje, desplazamiento = 0;
	int socketWorker = conectarseAWorker(puerto, ip);
	if (socketWorker != -1) {
		buffer = serializarSolicitudArchivo(nombreArchivoTemp,
				largoNombreArchTemp, &largoBuffer);
		tamanioMensaje = largoBuffer + sizeof(t_header);
		bufferMensaje = malloc(tamanioMensaje);
		header.id = SOLICITUD_WORKER;
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
		return socketWorker;
	} else {
		return socketWorker;
	}

}

int conectarseAWorker(int puerto, char* ip) {

	struct sockaddr_in direccionWorker;

	direccionWorker.sin_family = AF_INET;
	direccionWorker.sin_port = htons(puerto);
	direccionWorker.sin_addr.s_addr = inet_addr(ip);
	int socketWorker;

	socketWorker = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(socketWorker, (struct sockaddr *) &direccionWorker,
			sizeof(struct sockaddr)) != 0) {
		log_info(workerLogger,"Fallo conexion a worker %s:",ip);
		perror("fallo la conexion al worker");
		return -1;
	} else {
		log_info(workerLogger,"Se conecto a worker %s:",ip);
	}
	return socketWorker;
}

int conectarseAFilesystem(int puerto, char* ip) {

	struct sockaddr_in direccionFilesystem;

	direccionFilesystem.sin_family = AF_INET;
	direccionFilesystem.sin_port = htons(puerto);
	direccionFilesystem.sin_addr.s_addr = inet_addr(ip);
	int socketFilesystem;

	socketFilesystem = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(socketFilesystem, (struct sockaddr *) &direccionFilesystem,
			sizeof(struct sockaddr)) != 0) {
		log_info(workerLogger,"Fallo conexion a filesystem para almacenado final %s:",ip);
		perror("fallo la conexion a filesystem");
		return -1;
	} else {
		log_info(workerLogger,"Se conecto a filesystem");
	}
	return socketFilesystem;
}
void* serializarSolicitudArchivo(char* nombreArchTemp, int largoNombreArchTemp,
		int* largoBuffer) {
	void* buffer;
	int desplazamiento = 0;
	int tamanioBuffer = largoNombreArchTemp + sizeof(uint32_t);
	buffer = malloc(tamanioBuffer);
	largoNombreArchTemp++;
	memcpy(buffer + desplazamiento, &largoNombreArchTemp, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	string_append(&nombreArchTemp, "\0");
	memcpy(buffer + desplazamiento, nombreArchTemp, largoNombreArchTemp);
	desplazamiento += largoNombreArchTemp;

	*largoBuffer = desplazamiento;

	return buffer;
}

char* deserializarSolicitudArchivo(void* buffer) {
	int largoNombreArchTemp;
	char*nombreArchTemp;
	int desplazamiento = 0, bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&largoNombreArchTemp, buffer + desplazamiento, bytesACopiar);
	desplazamiento += bytesACopiar;
	nombreArchTemp = malloc(largoNombreArchTemp);

	bytesACopiar = largoNombreArchTemp;
	memcpy(nombreArchTemp, buffer + desplazamiento, bytesACopiar);
	desplazamiento += bytesACopiar;

	return nombreArchTemp;
}

void responderSolicitudArchivoWorker(char* nombreArchTemp, int socketWorker) {
	char* pathTemporales = string_new();
	string_append(&pathTemporales, RUTA_TEMPORALES);
	char* rutaArchivo = string_new();
	int validacion, desplazamiento = 0;
	int tamanioBuffer;
	char*contenidoArch;
	void*buffer;
	void*bufferMensaje;
	t_header header;
	validacion = verificarExistenciaArchTemp(nombreArchTemp, pathTemporales);
	if (validacion == 2) {
		log_info(workerLogger,"No se encontro archivo solicitado por encargado: %s",nombreArchTemp);
		header.id = ERROR_ARCHIVO_NO_ENCONTRADO;
		header.tamanioPayload = 0;
		buffer = malloc(sizeof(uint32_t) * 2);
		memcpy(buffer + desplazamiento, &header.id, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(buffer + desplazamiento, &header.tamanioPayload,
				sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		enviarPorSocket(socketWorker, buffer, 0);
	} else {
		log_info(workerLogger,"Se encontro archivo solicitado por encargado: %s",nombreArchTemp);
		string_append(&rutaArchivo, pathTemporales);
		string_append(&rutaArchivo, nombreArchTemp);

		tamanioBuffer = devolverTamanioArchivo(rutaArchivo);
		contenidoArch = obtenerContenidoArchivo(rutaArchivo);
		header.id = RECIBIR_ARCH_TEMP;
		header.tamanioPayload = tamanioBuffer;
		bufferMensaje = malloc(tamanioBuffer + sizeof(t_header));
		buffer = malloc(tamanioBuffer);
		memcpy(buffer, contenidoArch, tamanioBuffer);

		memcpy(bufferMensaje, &header.id, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(bufferMensaje + desplazamiento, &header.tamanioPayload,
				sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(bufferMensaje + desplazamiento, buffer, header.tamanioPayload);

		enviarPorSocket(socketWorker, bufferMensaje, header.tamanioPayload);
		log_info(workerLogger,"Se responde solicitud de encargado");
	}
	free(buffer);
	free(bufferMensaje);
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
	return tam;
}

char* obtenerContenidoArchivo(char*rutaArchivo) {
	char* buffer;
	int file = open(rutaArchivo, O_RDWR);
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

int verificarExistenciaArchTemp(char* nombreArchTemp, char* pathTemporales) {

	char* pathArchTemp = armarRutaGuardadoTemp(nombreArchTemp);
	if (access(pathArchTemp, F_OK) != -1) {
		return 1;
	}
	return 2;
}

t_infoTransformacion* deserializarInfoTransformacion(void* buffer) {
	t_infoTransformacion* infoTransformacion = malloc(
			sizeof(t_infoTransformacion));
	uint32_t desplazamiento = 0, bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoTransformacion->numBloque, buffer + desplazamiento,
			bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoTransformacion->bytesOcupados, buffer + desplazamiento,
			bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoTransformacion->largoNombreArchTemp, buffer + desplazamiento,
			bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = infoTransformacion->largoNombreArchTemp;
	infoTransformacion->nombreArchTemp = malloc(
			infoTransformacion->largoNombreArchTemp);
	memcpy(infoTransformacion->nombreArchTemp, buffer + desplazamiento,
			bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoTransformacion->largoArchTransformador, buffer + desplazamiento,
			bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = infoTransformacion->largoArchTransformador + 1;
	infoTransformacion->archTransformador = malloc(
			infoTransformacion->largoArchTransformador);
	memcpy(infoTransformacion->archTransformador, buffer + desplazamiento,
			infoTransformacion->largoArchTransformador);
	string_append(&infoTransformacion->archTransformador, "\0");

	desplazamiento += bytesACopiar;

	free(buffer);
	return infoTransformacion;
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
	string_append(&infoReduccionLocal->rutaArchReducidoLocal, "\0");
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
	string_append(&infoReduccionLocal->archReductor, "\0");
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

		bytesACopiar = temporal->largoRutaTemporalTransformacion;
		temporal->rutaTemporalTransformacion = malloc(
				temporal->largoRutaTemporalTransformacion);
		memcpy(temporal->rutaTemporalTransformacion, buffer + desplazamiento,
				bytesACopiar);
		desplazamiento += bytesACopiar;

		list_add(infoReduccionLocal->archTemporales,
				temporal->rutaTemporalTransformacion);

	}
	free(buffer);
	return infoReduccionLocal;

}

t_infoReduccionGlobal* deserializarInfoReduccionGlobal(void*buffer) {
	t_infoReduccionGlobal* infoReduccionGlobal = malloc(
			sizeof(t_infoReduccionGlobal));
	uint32_t bytesACopiar, desplazamiento = 0, i = 0;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoReduccionGlobal->largoRutaArchivoLocal, buffer + desplazamiento,
			bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = infoReduccionGlobal->largoRutaArchivoLocal;
	infoReduccionGlobal->rutaArchivoLocal = malloc(
			infoReduccionGlobal->largoRutaArchivoLocal);
	memcpy(infoReduccionGlobal->rutaArchivoLocal, buffer + desplazamiento,
			bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoReduccionGlobal->largoRutaArchivoTemporalFinal,
			buffer + desplazamiento, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = infoReduccionGlobal->largoRutaArchivoTemporalFinal;
	infoReduccionGlobal->rutaArchivoTemporalFinal = malloc(
			infoReduccionGlobal->largoRutaArchivoTemporalFinal);
	memcpy(infoReduccionGlobal->rutaArchivoTemporalFinal,
			buffer + desplazamiento, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoReduccionGlobal->largoArchivoReductor, buffer + desplazamiento,
			bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = infoReduccionGlobal->largoArchivoReductor;
	infoReduccionGlobal->archivoReductor = malloc(
			infoReduccionGlobal->largoArchivoReductor);
	memcpy(infoReduccionGlobal->archivoReductor, buffer + desplazamiento,
			bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoReduccionGlobal->cantidadNodos, buffer + desplazamiento,
			bytesACopiar);
	desplazamiento += bytesACopiar;

	infoReduccionGlobal->nodosAConectar = list_create();
	for (i = 0; i < infoReduccionGlobal->cantidadNodos; i++) {
		t_datosNodoAEncargado* nodo = malloc(sizeof(t_datosNodoAEncargado));
		bytesACopiar = sizeof(uint32_t);
		memcpy(&nodo->puerto, buffer + desplazamiento, bytesACopiar);
		desplazamiento += bytesACopiar;

		bytesACopiar = sizeof(uint32_t);
		memcpy(&nodo->largoIp, buffer + desplazamiento, bytesACopiar);
		desplazamiento += bytesACopiar;

		bytesACopiar = nodo->largoIp;
		nodo->ip = malloc(nodo->largoIp);
		memcpy(nodo->ip, buffer + desplazamiento, bytesACopiar);
		desplazamiento += bytesACopiar;

		bytesACopiar = sizeof(uint32_t);
		memcpy(&nodo->largoRutaArchivoReduccionLocal, buffer + desplazamiento,
				bytesACopiar);
		desplazamiento += bytesACopiar;

		bytesACopiar = nodo->largoRutaArchivoReduccionLocal;
		nodo->rutaArchivoReduccionLocal = malloc(
				nodo->largoRutaArchivoReduccionLocal);
		memcpy(nodo->rutaArchivoReduccionLocal, buffer + desplazamiento,
				bytesACopiar);
		//string_append(&nodo->rutaArchivoReduccionLocal,"\0");
		desplazamiento += bytesACopiar;

		list_add(infoReduccionGlobal->nodosAConectar, nodo);
	}
	free(buffer);
	return infoReduccionGlobal;
}

t_infoGuardadoFinal* deserializarInfoGuardadoFinal(void* buffer) {
	t_infoGuardadoFinal* infoGuardadoFinal = malloc(
			sizeof(t_infoGuardadoFinal));
	uint32_t bytesACopiar, desplazamiento = 0;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoGuardadoFinal->largoRutaTemporal, buffer + desplazamiento,
			bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = infoGuardadoFinal->largoRutaTemporal;
	infoGuardadoFinal->rutaTemporal = malloc(
			infoGuardadoFinal->largoRutaTemporal);
	memcpy(infoGuardadoFinal->rutaTemporal, buffer + desplazamiento,
			bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoGuardadoFinal->largoRutaArchFinal, buffer + desplazamiento,
			bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = infoGuardadoFinal->largoRutaArchFinal;
	infoGuardadoFinal->rutaArchFinal = malloc(
			infoGuardadoFinal->largoRutaArchFinal);
	memcpy(infoGuardadoFinal->rutaArchFinal, buffer + desplazamiento,
			bytesACopiar);
	desplazamiento += bytesACopiar;

	return infoGuardadoFinal;

}

void guardadoFinalEnFilesystem(t_infoGuardadoFinal* infoGuardadoFinal,
		int socketMaster) {
	char*contenidoArchTempFinal;
	int tamanioArchTempFinal;
	char* rutaTemp;
	int respuestaFileSystem;
	rutaTemp = armarRutaGuardadoTemp(infoGuardadoFinal->rutaTemporal);
	tamanioArchTempFinal = devolverTamanioArchivo(rutaTemp);
	contenidoArchTempFinal = obtenerContenidoArchivo(rutaTemp);
	void* buffer;
	void* bufferMensaje;
	t_header header;
	int largoBuffer, tamanioMensaje, desplazamiento = 0;
	int socketFilesystem = conectarseAFilesystem(PUERTO_FILESYSTEM,
			IP_FILESYSTEM);
	if (socketFilesystem != -1) {
		buffer = serializarInfoGuardadoFinal(tamanioArchTempFinal,
				contenidoArchTempFinal, infoGuardadoFinal, &largoBuffer);
		tamanioMensaje = largoBuffer;
		bufferMensaje = malloc(tamanioMensaje + sizeof(t_header));
		header.id = GUARDAR_FINAL;
		header.tamanioPayload = largoBuffer;

		memcpy(bufferMensaje, &header.id, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(bufferMensaje + desplazamiento, &header.tamanioPayload,
				sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(bufferMensaje + desplazamiento, buffer, header.tamanioPayload);

		enviarPorSocket(socketFilesystem, bufferMensaje, tamanioMensaje);
		log_info(workerLogger, "Envio archivo para guardarse en filesystem %s",
				infoGuardadoFinal->rutaArchFinal);
		respuestaFileSystem = recibirRespuestaFileSystem(socketFilesystem);
		if (respuestaFileSystem == RESPUESTA_FS_OK) {
			notificarAMaster(GUARDADO_OK_EN_FS, socketMaster);
			log_info(workerLogger,
					"Guardado final en filesystem ok de archivo %s",
					infoGuardadoFinal->rutaArchFinal);
		} else {
			notificarAMaster(FALLO_GUARDADO_FINAL, socketMaster);
			log_info(workerLogger, "Fallo en guardado final de archivo %s",
					infoGuardadoFinal->rutaArchFinal);
		}
	} else {
		notificarAMaster(FALLO_GUARDADO_FINAL, socketMaster);
		log_info(workerLogger, "Fallo en guardado final de archivo %s",
				infoGuardadoFinal->rutaArchFinal);
	}
	free(buffer);
	free(bufferMensaje);
}

int recibirRespuestaFileSystem(int socketFileSystem) {
	t_header header;
	recibirHeader(socketFileSystem, &header);

	if (header.id == RESPUESTA_FS_OK) {
		return RESPUESTA_FS_OK;
		log_info(workerLogger,"Filesystem almaceno correctamente el archivo");
	} else {
		return ERROR_GUARDADO_FINAL;
	}

}

void* serializarInfoGuardadoFinal(int tamanioArchTempFinal,
		char* contenidoArchTempFinal, t_infoGuardadoFinal* infoGuardadoFinal,
		int* largoBuffer) {
	void*buffer;
	int desplazamiento = 0;
	int tamanioBuffer = infoGuardadoFinal->largoRutaArchFinal
			+ tamanioArchTempFinal + (sizeof(uint32_t) * 2);
	buffer = malloc(tamanioBuffer);

	memcpy(buffer + desplazamiento, &infoGuardadoFinal->largoRutaArchFinal,
			sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, infoGuardadoFinal->rutaArchFinal,
			infoGuardadoFinal->largoRutaArchFinal);
	desplazamiento += infoGuardadoFinal->largoRutaArchFinal;
	memcpy(buffer + desplazamiento, &tamanioArchTempFinal, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(buffer + desplazamiento, contenidoArchTempFinal,
			tamanioArchTempFinal);
	desplazamiento += tamanioArchTempFinal;

	*largoBuffer = desplazamiento;
	return buffer;
}

void notificarAMaster(int idNotificacion, int socketMaster) {
	t_header header;
	header.id = idNotificacion;
	header.tamanioPayload = 0;
	enviarPorSocket(socketMaster, &header, 0);
}

void verificarExistenciaCarpetaLogs() {
	DIR* directorio;
	char*pathDirectorio;
	pathDirectorio = malloc(100);
	char cwd[100];
	char*linea = string_new();
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		string_append(&pathDirectorio, cwd);
		string_append(&pathDirectorio, "/logs");
		directorio = opendir(pathDirectorio);
		if (directorio == NULL) {
			string_append(&linea, "mkdir ");
			string_append(&linea, pathDirectorio);
			system(linea);
		} else {
			closedir(directorio);
		}
	}
	free(pathDirectorio);
	free(linea);
}
