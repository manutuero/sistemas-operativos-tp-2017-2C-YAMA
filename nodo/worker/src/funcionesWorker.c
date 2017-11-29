#include "funcionesWorker.h"

void crearLogger() {
	char *pathLogger = string_new();
	char cwd[1024];
	string_append(&pathLogger, getcwd(cwd, sizeof(cwd)));
	string_append(&pathLogger, "/workerLogs.log");

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
	if (config_has_property(config, "RUTA_DATABIN")) {
		RUTA_TEMPORALES = config_get_string_value(config, "RUTA_TEMPORALES");
	}


	printf("\nIP Filesystem: %s\n", IP_FILESYSTEM);
	printf("\nPuerto Filesystem: %d\n", PUERTO_FILESYSTEM);
	printf("\nID Nodo %s\n", ID_NODO);
	printf("\nPuerto Worker %d\n", PUERTO_WORKER);
	printf("\nPuerto Datanode %d\n", PUERTO_DATANODE);
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
	/*char mode[] = "0777";
	 char buf[100] = "/home/utnso/Escritorio/archivoSalida";
	 int i;*/

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
	char *regionDeMapeo, *data = malloc(bytesALeer + 1);

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

void realizarTransformacion(t_infoTransformacion* infoTransformacion,int socketMaster) {
	int respuesta;
	char*rutaGuardadoTemp=string_new();
	char* rutaArchTransformador;
	char*rutaDataBloque=string_new();
	string_append(&rutaDataBloque,"/home/utnso/thePonchos");
	string_append(&rutaDataBloque,infoTransformacion->nombreArchTemp);
	string_append(&rutaDataBloque,"D");
	FILE* dataBloque=fopen(rutaDataBloque,"w+");
	FILE*archivoTemp;
	rutaArchTransformador = guardarArchScript(
			infoTransformacion->archTransformador,infoTransformacion->nombreArchTemp);
	char* bloque=malloc(UN_BLOQUE);
	char*data=malloc(infoTransformacion->bytesOcupados);
	bloque = getBloque(infoTransformacion->numBloque);
	data = memcpy(data, bloque, infoTransformacion->bytesOcupados);
	txt_write_in_file(dataBloque,data);
	char* lineaAEjecutar = string_new();
	string_append(&data, "\0");
	rutaGuardadoTemp=armarRutaGuardadoTemp(infoTransformacion->nombreArchTemp);
	archivoTemp = fopen(rutaGuardadoTemp, "w+");
	string_append_with_format(&lineaAEjecutar, "cat %s | %s | sort > %s",
			rutaDataBloque, rutaArchTransformador, rutaGuardadoTemp);

	respuesta = system(lineaAEjecutar);

	//registrarOperacionEnLogs(repuesta)
	fclose(dataBloque);
	fclose(archivoTemp);
	remove(rutaArchTransformador);
	remove(rutaDataBloque);
	//free(bloque);
	//free(data);
	notificarAMaster(TRANSFORMACION_OK, socketMaster);

	if(respuesta!=-1){
	log_info(workerLogger,"Transformacion del bloque d% realizada",infoTransformacion->numBloque);
	}else {
	log_info(workerLogger,"No se pudo realizar la transformacion en el bloque",infoTransformacion->numBloque);
	}

}

char* armarRutaGuardadoTemp(char*nombreTemp) {
	char*rutaGuardadoTemp=string_new();
	string_append(&rutaGuardadoTemp,RUTA_TEMPORALES);
	string_append(&rutaGuardadoTemp,nombreTemp);
	return rutaGuardadoTemp;

}

char *guardarArchScript(char*contenidoArchivoScript,char* nombreArchTemp) {
	char*rutaArchScritp = string_new();
	char modo[]="0777";
	int permiso;
	char* nombreSolo=string_new();
	nombreSolo=string_substring_from(nombreArchTemp,4);
	string_append(&rutaArchScritp,
			"/home/utnso/thePonchos/tmp/scripts");
	string_append(&rutaArchScritp,nombreSolo);
	FILE*arch = fopen(rutaArchScritp, "w+");
	permiso=strtol(modo,0,8);
	chmod(rutaArchScritp,permiso);
	txt_write_in_file(arch, contenidoArchivoScript);
	fclose(arch);
	return rutaArchScritp;
}

/*char* armarNombreConPathTemp(char* nombreTemp){
 char * nombreArchTemp= string_new();
 string_append(&nombreArchTemp,pathTemporales);
 string_append(&nombreArchTemp,nombreTemp);
 return nombreArchTemp;
 }*/

void realizarReduccionLocal(t_infoReduccionLocal* infoReduccionLocal,int socketMaster) {
	//verificarExistenciaPathTemp(pathTemporales);
	char*rutaReducidoLocal=string_new();
	char*rutaArchReductor=string_new();
	rutaArchReductor = guardarArchScript(infoReduccionLocal->archReductor,infoReduccionLocal->rutaArchReducidoLocal);
	char*rutaArchConcat=string_new();
		rutaArchConcat=armarRutaGuardadoTemp(infoReduccionLocal->rutaArchReducidoLocal);
		string_append(&rutaArchConcat,"C");
	FILE*archivoConcat=fopen(rutaArchConcat,"w+");
	FILE* archivoReduccionLocal;
	rutaReducidoLocal=armarRutaGuardadoTemp(infoReduccionLocal->rutaArchReducidoLocal);
	archivoReduccionLocal = fopen(rutaReducidoLocal,"w+");
	char*lineaAEjecutar = string_new();
	int i;
	int resultado;
	FILE* archivo;
	char linea[LARGO_MAX_LINEA];
	for (i = 0; i < infoReduccionLocal->cantidadTransformaciones; i++) {
		archivo = fopen((char*)list_get(infoReduccionLocal->archTemporales, i), "r+");
		while (fgets(linea, LARGO_MAX_LINEA, archivo) != NULL) {
			txt_write_in_file(archivoConcat, linea);
		}
		fclose(archivo);
	}

	string_append_with_format(&lineaAEjecutar, "cat %s | sort | %s > %s",
			rutaArchConcat, rutaArchReductor,
			rutaReducidoLocal);

	resultado = system(lineaAEjecutar);
	fclose(archivoConcat);
	remove(rutaArchConcat);
	remove(rutaArchReductor);
	fclose(archivoReduccionLocal);

	if(resultado!=-1) {
		notificarAMaster(REDUCCION_LOCAL_OK, socketMaster);
		log_info(workerLogger,"Reduccion local realizada,archivo %s generado",infoReduccionLocal->rutaArchReducidoLocal);

	}
		notificarAMaster(ERROR_REDUCCION,socketMaster);
	}

void realizarReduccionGlobal(t_infoReduccionGlobal* infoReduccionGlobal,int socketMaster) {
	char* lineaAEjecutar = string_new();
	char* contenidoArchRecibido = string_new();
	int i, socketDePedido, encontrado;
	int resultado;
	FILE* archivoReduccionGlobal;
	char*rutaArchAAparear=string_new();
	char*rutaArchApareado=string_new();
	string_append(&rutaArchAAparear,infoReduccionGlobal->rutaArchivoTemporalFinal);
	string_append(&rutaArchAAparear,"A");
	string_append(&rutaArchApareado,infoReduccionGlobal->rutaArchivoTemporalFinal);
	string_append(&rutaArchApareado,"AA");
	FILE* archAAparear = fopen(rutaArchAAparear, "w+");
	FILE*archivoApareado = fopen(rutaArchApareado, "w+");
	archivoReduccionGlobal = fopen(
			infoReduccionGlobal->rutaArchivoTemporalFinal, "r+");
	for (i = 0; i < list_size(infoReduccionGlobal->nodosAConectar); i++) {
		t_datosNodoAEncargado infoArchivo = *(t_datosNodoAEncargado*) list_get(
				infoReduccionGlobal->nodosAConectar, i);
		FILE* archivoRecibido = fopen("tempRecibido", "w+");
		socketDePedido = solicitarArchivoAWorker(infoArchivo.ip,
				infoArchivo.puerto, infoArchivo.rutaArchivoReduccionLocal);
		if (socketDePedido != -1) {
			contenidoArchRecibido = recibirArchivoTemp(socketDePedido,
					&encontrado);
			txt_write_in_file(archivoRecibido, contenidoArchRecibido);
			if (encontrado != 0) {

				aparearArchivos(archAAparear, archivoRecibido, archivoApareado);
				exit(1);
			} else {
				notificarAMaster(ERROR_REDUCCION_GLOBAL,socketMaster);
				exit(1);
			}
		} else {
			notificarAMaster(ERROR_REDUCCION_GLOBAL,socketMaster);
		}

		fclose(archivoRecibido);
	}

	copiarContenidoDeArchivo(archivoReduccionGlobal, archivoApareado);

	string_append_with_format(&lineaAEjecutar, "cat %s | %s > %s",
			archivoReduccionGlobal, infoReduccionGlobal->archivoReductor,
			infoReduccionGlobal->rutaArchivoTemporalFinal);
	resultado = system(lineaAEjecutar);

	notificarAMaster(REDUCCION_GLOBAL_OK, socketMaster);
	fclose(archAAparear);
	fclose(archivoApareado);

	log_info(workerLogger,"Reduccion Global realizada, archivo %s generado",infoReduccionGlobal->rutaArchivoTemporalFinal);
}

void aparearArchivos(FILE* archAAparear, FILE* archivoRecibido,
		FILE* archivoApareado) {
	t_regArch regArch1 = malloc(LARGO_MAX_LINEA);
	t_regArch regArch2 = malloc(LARGO_MAX_LINEA);
	bool finArch1 = false;
	bool finArch2 = false;
	copiarContenidoDeArchivo(archivoApareado, archAAparear);

	//regArch1.linea=fgets(linea,LARGO_MAX_LINEA,arch1);
	//regArch2.linea=fgets(linea,LARGO_MAX_LINEA,arch2);
	leerRegArchivo(archAAparear, regArch1, &finArch1);
	leerRegArchivo(archivoRecibido, regArch2, &finArch2);
	while (!finArch1 && !finArch2) {
		if (strcmp(regArch1, regArch2) < 0) {
			txt_write_in_file(archivoApareado, regArch1);
			//fwrite(regArch1.linea,sizeof(char),strlen(regArch1.linea),archFinal);
			leerRegArchivo(archAAparear, regArch1, &finArch1);
		} else {
			txt_write_in_file(archivoApareado, regArch2);
			//fwrite(regArch2.linea,sizeof(char),strlen(regArch2.linea),archFinal);
			leerRegArchivo(archivoRecibido, regArch2, &finArch2);
		}
	}
	txt_write_in_file(archivoApareado, "\n");
	if (!finArch1) {
		while (!finArch1) {
			txt_write_in_file(archivoApareado, regArch1);
			//fwrite(regArch1.linea,sizeof(char),strlen(regArch1.linea),archFinal);
			leerRegArchivo(archAAparear, regArch1, &finArch1);
		}
		//txt_write_in_file(archFinal,"\n");
	} else {
		while (!finArch2) {
			txt_write_in_file(archivoApareado, regArch2);
			//fwrite(regArch2.linea,sizeof(char),strlen(regArch2.linea),archFinal);
			leerRegArchivo(archivoRecibido, regArch2, &finArch2);
		}
		//txt_write_in_file(archFinal,"\n");
	}

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
	t_regArch regArchivoACopiar = malloc(LARGO_MAX_LINEA);
	while (!(feof(archivoACopiar))) {
		fgets(regArchivoACopiar, LARGO_MAX_LINEA, archivoACopiar);
		txt_write_in_file(archivoCopiado, regArchivoACopiar);
	}

}

char* recibirArchivoTemp(int socketDePedido, int* encontrado) {
	void*buffer;
	t_header header;
	recibirHeader(socketDePedido, &header);
	buffer = malloc(header.tamanioPayload);
	recibirPorSocket(socketDePedido, buffer, header.tamanioPayload);
	char*archTemporal = malloc(header.tamanioPayload);

	if (header.id == ERROR_ARCHIVO_NO_ENCONTRADO) {
		encontrado = 0;
	} else {
		archTemporal = deserializarRecepcionArchivoTemp(buffer);
	}
	return archTemporal;
	free(archTemporal);
}

char* deserializarRecepcionArchivoTemp(void* buffer) {

	char* archTemporal = string_new();
	int desplazamiento = 0;
	int bytesACopiar;
	int largoArchTemporal;
	bytesACopiar = sizeof(uint32_t);
	memcpy(&largoArchTemporal, buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += bytesACopiar;

	bytesACopiar = largoArchTemporal;
	memcpy(archTemporal, buffer + desplazamiento, largoArchTemporal);
	desplazamiento += bytesACopiar;
	return archTemporal;
}

int solicitarArchivoAWorker(char*ip, int puerto, char*nombreArchivoTemp) {
	void* buffer;
	void* bufferMensaje;
	t_header header;
	int largoBuffer, tamanioMensaje, desplazamiento = 0;
	int socketWorker = conectarseAWorker(puerto, ip);
	if (socketWorker != -1) {
		buffer = serializarSolicitudArchivo(nombreArchivoTemp, &largoBuffer);
		tamanioMensaje = largoBuffer;
		bufferMensaje = malloc(tamanioMensaje);
		header.id = SOLICITUD_WORKER;
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

		return socketWorker;
	} else {
		return socketWorker;
	}

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

int conectarseAFilesystem(int puerto, char* ip) {

	struct sockaddr_in direccionFilesystem;
	printf("puerto %d, ip %s\n", puerto, ip);

	direccionFilesystem.sin_family = AF_INET;
	direccionFilesystem.sin_port = htons(puerto);
	direccionFilesystem.sin_addr.s_addr = inet_addr(ip);
	//memset(&(direccionYama.sin_zero), '\0', 8);
	int socketFilesystem;

	socketFilesystem = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(socketFilesystem, (struct sockaddr *) &direccionFilesystem,
			sizeof(struct sockaddr)) != 0) {
		perror("fallo la conexion al worker");
		return -1;
	} else {
		printf("se conecto a un worker\n");
	}

	return socketFilesystem;
}
void* serializarSolicitudArchivo(char* nombreArchTemp, int* largoBuffer) {
	void* buffer;
	int desplazamiento = 0;
	int largoNombreArchTemp = strlen(nombreArchTemp);
	int tamanioBuffer = largoNombreArchTemp + sizeof(uint32_t);
	buffer = malloc(tamanioBuffer);

	memcpy(buffer + desplazamiento, &largoNombreArchTemp, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(buffer + desplazamiento, &nombreArchTemp, largoNombreArchTemp);
	desplazamiento += largoNombreArchTemp;

	*largoBuffer = desplazamiento;

	return buffer;
}

char* deserializarSolicitudArchivo(void* buffer) {
	int* largoNombreArchTemp;
	char*nombreArchTemp = string_new();
	int desplazamiento = 0, bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&largoNombreArchTemp, buffer + desplazamiento, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = *largoNombreArchTemp;
	memcpy(nombreArchTemp, buffer + desplazamiento, bytesACopiar);
	desplazamiento += bytesACopiar;

	return nombreArchTemp;
}

void responderSolicitudArchivoWorker(char* nombreArchTemp, int socketWorker) {
	char* pathTemporales =RUTA_TEMPORALES;
	char* rutaArchivo = string_new();
	int validacion, desplazamiento = 0;
	int tamanioBuffer;
	char*contenidoArch = string_new();
	void*buffer;
	void*bufferMensaje;
	t_header header;
	validacion = verificarExistenciaArchTemp(nombreArchTemp, pathTemporales);
	if (validacion == 1) {
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
		string_append(&rutaArchivo, pathTemporales);
		string_append(&rutaArchivo, nombreArchTemp);
		contenidoArch = obtenerContenidoArchivo(rutaArchivo);
		tamanioBuffer = devolverTamanioArchivo(rutaArchivo);
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

		enviarPorSocket(socketWorker, bufferMensaje,header.tamanioPayload);

	}
	free(buffer);
	free(bufferMensaje);

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

	if (access(nombreArchTemp, F_OK) != -1) {
		return 1;
	}

	return 2;
}
/*for(i=1; i < list_size(listaArchivos); i++){
 aparearArchivos(primerArchivo,list);
 }

 }*/
/*
 void recibirInfoOperacion(int socketMaster) {
 void* buffer;
 t_header header;
 recibirHeader(socketMaster,&header);
 buffer=malloc(header.tamanioPayload);
 recibirPorSocket(socketMaster,buffer,header.tamanioPayload);
 switch(header.id){

 case TRANSFORMACION:
 deserializarInfoTransformacion(buffer);
 break;
 case REDUCCION_LOCAL:
 deserializarInfoReduccionLocal(buffer);
 break;
 case REDUCCION_GLOBAL:
 deserializarInfoReduccionGlobal(buffer);
 break;
 }
 }*/

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

	bytesACopiar = infoTransformacion->largoArchTransformador+1;
	infoTransformacion->archTransformador = malloc(infoTransformacion->largoArchTransformador);
	memcpy(infoTransformacion->archTransformador, buffer + desplazamiento,
			infoTransformacion->largoArchTransformador);
	string_append(&infoTransformacion->archTransformador,"\0");

	desplazamiento += bytesACopiar;

	return infoTransformacion;
}

t_infoReduccionLocal* deserializarInfoReduccionLocal(void*buffer) {
	t_infoReduccionLocal* infoReduccionLocal = malloc(
			sizeof(t_infoReduccionLocal));
	uint32_t bytesACopiar, desplazamiento = 0, i = 0;

	puts("llegue a deserializar");
	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoReduccionLocal->largoRutaArchReducidoLocal,
			buffer + desplazamiento, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = infoReduccionLocal->largoRutaArchReducidoLocal;
	infoReduccionLocal->rutaArchReducidoLocal = malloc(
			infoReduccionLocal->largoRutaArchReducidoLocal);
	memcpy(infoReduccionLocal->rutaArchReducidoLocal, buffer + desplazamiento,
			bytesACopiar);
	string_append(&infoReduccionLocal->rutaArchReducidoLocal,"\0");
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
	string_append(&infoReduccionLocal->archReductor,"\0");
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
	int j;
	printf("%d\n",infoReduccionLocal->largoRutaArchReducidoLocal);
	printf("%s\n",infoReduccionLocal->rutaArchReducidoLocal);
	printf("%d\n",infoReduccionLocal->largoArchivoReductor);
	//printf("%s\n",infoReduccionLocal->archReductor);

	printf("%d\n",infoReduccionLocal->cantidadTransformaciones);
	for(j=0;j<infoReduccionLocal->cantidadTransformaciones;j++){
		printf("%s\n",(char*)list_get(infoReduccionLocal->archTemporales,j));
	}

	return infoReduccionLocal;

}

t_infoReduccionGlobal* deserializarInfoReduccionGlobal(void*buffer) {
	t_infoReduccionGlobal* infoReduccionGlobal = malloc(
			sizeof(t_infoReduccionGlobal));
	uint32_t bytesACopiar, desplazamiento = 0, i = 0;

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

	bytesACopiar = sizeof(infoReduccionGlobal->largoArchivoReductor);
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
	for (i = 0; i > infoReduccionGlobal->cantidadNodos; i++) {
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
		desplazamiento += bytesACopiar;

		list_add(infoReduccionGlobal->nodosAConectar, nodo);
	}
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
	infoGuardadoFinal->rutaArchFInal = malloc(
			infoGuardadoFinal->largoRutaArchFinal);
	desplazamiento += bytesACopiar;

	return infoGuardadoFinal;

}

void guardadoFinalEnFilesystem(t_infoGuardadoFinal* infoGuardadoFinal) {
	char*contenidoArchTempFinal;
	int tamanioArchTempFinal;
	tamanioArchTempFinal = devolverTamanioArchivo(
			infoGuardadoFinal->rutaTemporal);
	contenidoArchTempFinal = malloc(tamanioArchTempFinal);
	contenidoArchTempFinal = obtenerContenidoArchivo(
			infoGuardadoFinal->rutaTemporal);
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
		bufferMensaje = malloc(tamanioMensaje);
		header.id = GUARDAR_FINAL;
		header.tamanioPayload = largoBuffer;

		memcpy(bufferMensaje, &header.id, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(bufferMensaje + desplazamiento, &header.tamanioPayload,
				sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(bufferMensaje + desplazamiento, buffer, header.tamanioPayload);

		enviarPorSocket(socketFilesystem, bufferMensaje, tamanioMensaje);
		free(buffer);
		free(bufferMensaje);
	}
}

void* serializarInfoGuardadoFinal(int tamanioArchTempFinal,
		char* contenidoArchTempFinal, t_infoGuardadoFinal* infoGuardadoFinal,
		int* largoBuffer) {
	void*buffer;
	int desplazamiento = 0;
	;
	int tamanioBuffer = infoGuardadoFinal->largoRutaArchFinal
			+ tamanioArchTempFinal + (sizeof(uint32_t) * 2);
	buffer = malloc(tamanioBuffer);

	memcpy(buffer + desplazamiento, &infoGuardadoFinal->largoRutaArchFinal,
			sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer + desplazamiento, infoGuardadoFinal->rutaArchFInal,
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
