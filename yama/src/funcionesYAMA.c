/*
 * funcionesYAMA.c
 *
 *  Created on: 20/9/2017
 *      Author: utnso
 */

#include "funcionesYAMA.h"
#include "funcionesPlanificacion.h"

uint32_t masterID = 0, disponibilidadBase;
char* temp = "/tmp/";

void conectarseAFS() {

//	struct sockaddr_in direccionFS;

	direccionFS.sin_family = AF_INET;
	direccionFS.sin_port = htons(FS_PUERTO);
	direccionFS.sin_addr.s_addr = inet_addr(FS_IP);
	//memset(&(direccionYama.sin_zero), '\0', 8);

	socketFS = socket(AF_INET, SOCK_STREAM, 0);

	if (connect(socketFS, (struct sockaddr *) &direccionFS,
			sizeof(struct sockaddr)) != 0) {
		perror("fallo la conexion al fileSystem");
		exit(1);
	}

	printf("Conectado al filesystem con el socket %d\n", socketFS);

}

void cargarArchivoDeConfiguracion() {
	char* path = "YAMAconfig.cfg";
	char cwd[1024];
	char *pathArchConfig = string_from_format("%s/%s", getcwd(cwd, sizeof(cwd)),
			path);
	//t_config *config = config_create(pathArchConfig);
	config = config_create(pathArchConfig);

	if (!config) {
		perror("[ERROR]: No se pudo cargar el archivo de configuracion.");
		exit(EXIT_FAILURE);
	}

	FS_IP = string_new();
	if (config_has_property(config, "FS_IP")) {
		FS_IP = config_get_string_value(config, "FS_IP");
	}else
	{
		perror("[ERROR]: No se encontro la propiedad FS_IP");
		exit(EXIT_FAILURE);
	}

	if (config_has_property(config, "FS_PUERTO")) {
		FS_PUERTO = config_get_int_value(config, "FS_PUERTO");
	}else
	{
		perror("[ERROR]: No se encontro la propiedad FS_PUERTO");
		exit(EXIT_FAILURE);
	}

	if (config_has_property(config, "JOB")) {
		job = config_get_int_value(config, "JOB");
	}else
	{
		perror("[ERROR]: No se encontro la propiedad JOB");
		exit(EXIT_FAILURE);
	}

	if (config_has_property(config, "RETARDO_PLANIFICACION")) {
		RETARDO_PLANIFICACION = config_get_int_value(config, "RETARDO_PLANIFICACION");
	}else
	{
		perror("[ERROR]: No se encontro la propiedad RETARDO_PLANIFICACION");
		exit(EXIT_FAILURE);
	}

	if (config_has_property(config, "DISPONIBILIDAD_BASE")) {
		disponibilidadBase = config_get_int_value(config,"DISPONIBILIDAD_BASE");
	}else
	{
		perror("[ERROR]: No se encontro la propiedad DISPONIBILIDAD_BASE");
		exit(EXIT_FAILURE);
	}

	if (config_has_property(config, "PUERTO_FS_NODOS")) {
		PUERTO_FS_NODOS = config_get_int_value(config,"PUERTO_FS_NODOS");
	}else
	{
		perror("[ERROR]: No se encontro la propiedad PUERTOS_FS_NODOS");
		exit(EXIT_FAILURE);
	}

	if (config_has_property(config, "PUERTO_MASTERS")) {
		PUERTO_MASTERS = config_get_int_value(config,"PUERTO_MASTERS");
	}else
	{
		perror("[ERROR]: No se encontro la propiedad PUERTO_MASTERS");
		exit(EXIT_FAILURE);
	}

	printf("Puerto: %d\n", PUERTO_MASTERS);
	printf("IP FS: %s\n", FS_IP);
	printf("PUERTO FS: %d\n", FS_PUERTO);
	printf("Job actual: %d\n", job);
}

void yamaEscuchando() {

	//struct sockaddr_in direccionDelServidorKernel;
	direccionYama.sin_family = AF_INET;
	direccionYama.sin_port = htons(PUERTO_MASTERS);
	direccionYama.sin_addr.s_addr = INADDR_ANY;
	//memset(&(direccionYama.sin_zero), '\0', 8);  // Se setea el resto del array de addr_in en 0

	int activado = 1;

	socketMasters = socket(AF_INET, SOCK_STREAM, 0);
	// Permite reutilizar el socket sin que se bloquee por 2 minutos
	if (setsockopt(socketMasters, SOL_SOCKET, SO_REUSEADDR, &activado,
			sizeof(activado)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	// Se enlaza el socket al puerto
	if (bind(socketMasters, (struct sockaddr *) &direccionYama,
			sizeof(struct sockaddr)) != 0) {
		perror("No se pudo conectar");
		exit(1);
	}
	// Se pone a escuchar el servidor kernel
	if (listen(socketMasters, 10) == -1) {
		perror("listen");
		exit(1);
	}
}

void escucharMasters() {

	fd_set readfds, auxRead;
	int tamanioDir = sizeof(direccionYama);
	char* buffer;
	int bytesRecibidos, maxPuerto, i, nuevoSocket,opt = 1;
	FD_ZERO(&readfds);
	FD_ZERO(&auxRead);
	FD_SET(socketMasters, &auxRead);

	if (setsockopt(socketMasters, SOL_SOCKET, SO_REUSEADDR, (char *) &opt,
				sizeof(opt)) < 0) {
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}




	maxPuerto = socketMasters;
	inicializarWorkers(); //inicializa antes de entrar a la plani. sacar cuando este la conexion de nodos 
	printf("escuchando masters\n");
	while (1) {

		readfds = auxRead;
		sleep(1);
		if (select(maxPuerto + 1, &readfds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(1);
		}

		for (i = 0; i <= maxPuerto; i++) {
			if (FD_ISSET(i, &readfds)) {
				if (i == socketMasters) {
					if ((nuevoSocket = accept(socketMasters,(void*) &direccionYama, (socklen_t *)&tamanioDir)) <= 0)
						perror("accept");
					else {

						printf("Entro una conexion por el puerto %d\n",
								nuevoSocket);
						FD_SET(nuevoSocket, &auxRead);

						t_pedidoTransformacion* rutas;
						rutas = malloc(sizeof(t_pedidoTransformacion));

						recibirRutaDeArchivoAProcesar(nuevoSocket,&rutas);
						free(rutas->nombreArchivo);
						free(rutas->nombreArchivoGuardadoFinal);
						free(rutas);
						//recive la ruta del archivo
						if (nuevoSocket > maxPuerto)
							maxPuerto = nuevoSocket;
					}
				} else {//header
					t_header headerResp;
					//headerResp.tamanioPayload = 0;
					bytesRecibidos = recibirHeader(i, &headerResp);

					if (bytesRecibidos < 0) {
						perror("Error menor a 0");
						shutdown(i, 2);
						FD_CLR(i, &readfds);
						free(buffer);
					}
					else if (bytesRecibidos == 0) {
						//printf("Se desconecto del fileSystem el socket %d", i);
						//printf("%d\n",headerResp.id);
						FD_CLR(i, &readfds);
						shutdown(i, 2);
						//close(i);
					} else {
						char* temporal = malloc(headerResp.tamanioPayload+1);
						switch(headerResp.id){
						case 12:
							//char* temporal = malloc(headerResp.tamanioPayload+1);
							temporal[headerResp.tamanioPayload] = '\0';
							bytesRecibidos = recibirPorSocket(i,temporal, headerResp.tamanioPayload);

							if(bytesRecibidos==0)
							{
								printf("Fallo la coneccion con master");
								break;
							}

							printf("termino la trans del temporal %s\n",temporal);

							if(cambiarEstado(temporal,COMPLETADO))
								{
									printf("temporal %s\n",temporal);
									conseguirIdNodo(temporal,&headerResp);
									printf("nodo reduccion local: %d\n",headerResp.tamanioPayload);
									headerResp.id = 13;
									enviarPorSocket(i,&headerResp,0);
								}

							free(temporal);

							break;

						case 16:
						//	char* temporal = malloc(headerResp.tamanioPayload+1);
							temporal[headerResp.tamanioPayload] = '\0';
							bytesRecibidos = recibirPorSocket(i,temporal, headerResp.tamanioPayload);

							if(bytesRecibidos==0)
							{
								printf("Fallo la coneccion con master");
								break;
							}

							printf("termino la redlocal del temporal %s\n",temporal);


							if(cambiarEstado(temporal,COMPLETADO)){
								headerResp.id = 17;
								headerResp.tamanioPayload=0;
								enviarPorSocket(i,&headerResp,0);
							}

							free(temporal);
							break;

						case 20:
							//char* temporal = malloc(headerResp.tamanioPayload+1);
							temporal[headerResp.tamanioPayload] = '\0';
							bytesRecibidos = recibirPorSocket(i,temporal, headerResp.tamanioPayload);

							if(bytesRecibidos==0)
							{
								printf("Fallo la coneccion con master");
								break;
							}

							printf("termino la redglobal del temporal %s\n",temporal);
							uint32_t respuesta = 21;
							headerResp.id = 21;
							headerResp.tamanioPayload=0;
							enviarPorSocket(i,&headerResp,0);
							descargarWorkload(temporal);
							free(temporal);
							printf("%d",respuesta);
							break;

						case 103:
							printf("repreplanifica\n");
							t_pedidoTransformacion* pedido = malloc(sizeof(t_pedidoTransformacion));
							char *nombreTMP;
							int *desplazamiento;
							buffer = malloc(headerResp.tamanioPayload);
							bytesRecibidos=recibirPorSocket(i,buffer,headerResp.tamanioPayload);
							if(bytesRecibidos==0)
							{
								printf("Fallo la coneccion con master");
								break;
							}

							nombreTMP = deserializarNombreTMP(buffer,desplazamiento);
							pedido=deserializarRutasArchivos(buffer+*desplazamiento,1);

							//pedido = deserializarTresRutasArchivos(buffer,nombreTMP);
							t_tabla_estados* registro = encontrarRegistro(nombreTMP);

							if(registro->job==-1)
							{
								printf("No se puede replanificar, no se encuentra el registro\n");
							}
							else
							{
								t_job* jobMaster = malloc(sizeof(t_job));
								jobMaster->job = registro->job;
								jobMaster->idMaster = registro->master;
								jobMaster->socketMaster = i;
								jobMaster->replanifica = 1;
								printf("repre2\n");
								rePrePlanificacion(pedido->nombreArchivo,pedido->nombreArchivoGuardadoFinal,nombreTMP, jobMaster);
							}
								break;

						case 104:
							printf("Se procede a terminar el trabajo en estado erroneo");
							eliminarJob(temporal);
							break;

						case 107:
							printf("Se procede a actualizar trabajo a error tarea por fallo guardado final en yamafs");
							eliminarJob(temporal);
							break;

						default:
							printf("Header ID erroneo");
						}

					}
				}
			}
		}
	}
}

void escuchaActualizacionesNodos() {

	t_header *header;
	t_infoNodos infoNodo;
	header = malloc(sizeof(t_header));
	struct addrinfo hints;
	struct addrinfo *serverInfo;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	char* puertoFsNodos = string_itoa(PUERTO_FS_NODOS);
	getaddrinfo(NULL, puertoFsNodos, &hints, &serverInfo); // Notar que le pasamos NULL como IP, ya que le indicamos que use localhost men AI_PASSIVE
	int socketYamaNodos;
	socketYamaNodos =nuevoSocket();
	//socket(serverInfo->ai_family, serverInfo->ai_socktype,serverInfo->ai_protocol);

	int activado=1;
	if (setsockopt(socketYamaNodos, SOL_SOCKET, SO_REUSEADDR, &activado,
				sizeof(activado)) == -1) {
			perror("setsockopt");
			exit(1);
		}

	bind(socketYamaNodos, serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo);
	listen(socketYamaNodos, 3); // IMPORTANTE: listen() es una syscall BLOQUEANTE.

	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	int socketCliente = accept(socketYamaNodos, (struct sockaddr *) &addr, &addrlen);

	int status = 1;		// Estructura que manjea el status de los recieve.
	while (status != 0) {
		status = recibirHeader(socketCliente, header);

		//status = recv(socketCliente, (void*) package, 8, 0);//8 ES EL TAMANIO DEL HEADER ENVIADOS DESDE YAMA
		if (status != 0) {

			void * actualizacionRecibida = malloc(header->tamanioPayload);

			status = recv(socketCliente, (void*) actualizacionRecibida,
					header->tamanioPayload, 0);

			
			infoNodo = deserializarActualizacion(actualizacionRecibida);

			if (header->id == 30) {
				workers[infoNodo.idNodo].habilitado = 1;
				workers[infoNodo.idNodo].puerto = infoNodo.puerto;
				workers[infoNodo.idNodo].ip = malloc(infoNodo.largoIp);
				strcpy(workers[infoNodo.idNodo].ip, infoNodo.IP);
				printf("Recibida info nodo:   %d", infoNodo.idNodo);
				puts("");
			} else {
				workers[infoNodo.idNodo].habilitado = 0;
			}

		
			

		}
	}
	close(socketCliente);
	close(socketMasters);
	return;
}

t_infoNodos deserializarActualizacion(void* mensaje) {

	int desplazamiento = 0, bytesACopiar = 0;
	t_infoNodos infoNodo;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoNodo.idNodo, mensaje + desplazamiento, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoNodo.puerto, mensaje + desplazamiento, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t); // recibimos longitud IP
	memcpy(&infoNodo.largoIp, mensaje + desplazamiento, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = infoNodo.largoIp;
	infoNodo.IP = malloc(infoNodo.largoIp);
	memcpy(infoNodo.IP, mensaje + desplazamiento, bytesACopiar);

	return infoNodo;
}

void mandarRutaAFS(const t_header* header, void* buffer) {
	//se lo manda a FS
	char* bufferFS;
	int desplazamiento = 0;
	bufferFS = malloc(header->tamanioPayload + 8);
	memcpy(bufferFS, &header->id, sizeof(header->id));
	desplazamiento += sizeof(header->id);
	memcpy(bufferFS + desplazamiento, &header->tamanioPayload,
			sizeof(header->tamanioPayload));
	memcpy(bufferFS + desplazamiento, buffer, header->tamanioPayload);
	enviarPorSocket(socketFS, bufferFS,	header->tamanioPayload);
	free(bufferFS);
}

int recibirRutaDeArchivoAProcesar(int socketMaster,t_pedidoTransformacion** ruta ){
	t_header header;
	void* buffer;
	int idMaster;

	if ((recibirHeader(socketMaster, &header)) <= 0) {
		perror("Error al recibir header");
		return -1;
	}
	else
	{
		buffer = malloc(header.tamanioPayload);
		recibirPorSocket(socketMaster, buffer, header.tamanioPayload);

		//se lo manda a FS
		//mandarRutaAFS(&header, buffer);

		*ruta = deserializarRutasArchivos(buffer,&idMaster);
		printf("me llegaron las rutas %s, %s\n",(*ruta)->nombreArchivo,(*ruta)->nombreArchivoGuardadoFinal);

		free(buffer);

		t_job* jobMaster = malloc(sizeof(t_job));
		jobMaster->idMaster = idMaster;
		jobMaster->job = job;
		jobMaster->socketMaster = socketMaster;
		jobMaster->pedidoTransformacion=**ruta;

		pthread_t hiloPeticionMaster;
		printf("job: %d\n", jobMaster->job);
		pthread_create(&hiloPeticionMaster, NULL, (void*) preplanificarJob,
				jobMaster);

		pthread_join(hiloPeticionMaster, NULL); //ver si despues se saca o se deja el join

		printf("salgo del hilo de plani\n");

		free(jobMaster);

		return 0;
	}

}

t_pedidoTransformacion* deserializarRutasArchivos(void* buffer, int *idMaster){
	t_pedidoTransformacion* rutaArchivos = malloc(sizeof(t_pedidoTransformacion));
	int desplazamiento = 0;

	memcpy(&rutaArchivos->largoArchivo, buffer+desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	rutaArchivos->nombreArchivo = malloc(rutaArchivos->largoArchivo);
	memcpy(rutaArchivos->nombreArchivo, buffer+desplazamiento, rutaArchivos->largoArchivo);
	desplazamiento += rutaArchivos->largoArchivo;

	memcpy(&rutaArchivos->largoArchivo2, buffer+desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	rutaArchivos->nombreArchivoGuardadoFinal= malloc(rutaArchivos->largoArchivo2);
	memcpy(rutaArchivos->nombreArchivoGuardadoFinal, buffer+desplazamiento, rutaArchivos->largoArchivo2);
	desplazamiento += rutaArchivos->largoArchivo2;

return rutaArchivos;
}

char* deserializarNombreTMP(void *buffer,int *desplazamiento)
{
	char *nombreTMP;
	int largo,desp=0;

	memcpy(&largo,buffer,sizeof(uint32_t));
	desp+=sizeof(uint32_t);
	nombreTMP = malloc(largo);

	memcpy(nombreTMP,buffer+desp,largo);

	desp+=largo;

	*desplazamiento=desp;
	return nombreTMP;
}

t_pedidoTransformacion* deserializarTresRutasArchivos(void *buffer,char *nombreTMP)
{
	t_pedidoTransformacion *pedido;
	pedido = malloc (sizeof(t_pedidoTransformacion));
	uint32_t largo,desplazamiento=0;
	char *tmpAux;

	//Nombre temporal
	memcpy(&largo,buffer+desplazamiento,sizeof(uint32_t));
	tmpAux = malloc(largo);
	desplazamiento+=sizeof(uint32_t);

	memcpy(tmpAux,buffer+desplazamiento,largo);
	desplazamiento+=largo;
	string_append(&nombreTMP,tmpAux);

	//Nombre archivo original
	memcpy(&largo,buffer+desplazamiento,sizeof(uint32_t));
	desplazamiento+=sizeof(uint32_t);

	pedido->largoArchivo = largo;
	pedido->nombreArchivo = malloc(largo);
	memcpy(pedido->nombreArchivo,buffer+desplazamiento,largo);
	desplazamiento+=largo;

	//Nombre archivo guardado final
	memcpy(&largo,buffer+desplazamiento,sizeof(uint32_t));
	desplazamiento+=sizeof(uint32_t);

	pedido->largoArchivo2=largo;
	pedido->nombreArchivoGuardadoFinal=malloc(largo);
	memcpy(pedido->nombreArchivoGuardadoFinal,buffer+desplazamiento,largo);

	return pedido;
}

void* obtenerBloquesDelArchivo(t_rutaArchivo* ruta) {
	t_header header;

	void* buffer;
	buffer = serializarRutaArchivo(&header, ruta); //esta en utils ya que lo voy a usar para Yama-fs
	int tamanioMensaje = header.tamanioPayload;
	enviarPorSocket(socketFS, buffer, tamanioMensaje);
	free(buffer);
	/*//Envio a FS y espero que me mande los bloques de ese archivo
	 t_header headerRespuesta;
	 recibirHeader(socketFS,&headerRespuesta);
	 t_bloques* bloques = recibirPaquete(socketFS,headerRespuesta);

	 return bloques;
	 */
	return buffer;
}

/* Crear la tabla de estados global */
void crearTablaDeEstados() {

	listaTablaEstados = list_create();

}

void conseguirIdNodo(char* temporal,t_header *header)
{
	t_tabla_estados *registro;
	int i;

	printf("%s\n", temporal);
	for(i=0;i<list_size(listaTablaEstados);i++)
	{
		registro = list_get(listaTablaEstados,i);
		if(sonIguales(registro->archivoTemp,temporal))
		{
			printf("archivo %s\n", registro->archivoTemp);
			printf("bloque %d\n", registro->bloque);
			printf("estado %d\n", registro->estado);
			printf("etapa %d\n", registro->etapa);
			printf("job %d\n", registro->job);
			printf("nodo %d\n", registro->nodo);
			printf("master %d\n", registro->master);
			printf("bloque archivo %d\n", registro->nroBloqueArch);
			header->tamanioPayload=registro->nodo;
			break;
		}

	}
}

void encargadoInterrupciones(int senial)
{
	int i;
	char *mensaje;
	t_tabla_estados *registro;
	t_log *logger;

	crearYAMALogger(logger);

	//signal(senial,SIG_IGN);

	if(list_size(listaTablaEstados)==0)
		log_info(logger,"NO HAY ACTIVIDAD QUE REGISTRAR");
	else
	{
		mensaje = string_new();
		for(i=0;i<list_size(listaTablaEstados);i++)
		{
			registro = list_get(listaTablaEstados,i);
			if((registro->estado==COMPLETADO)||(registro->estado==ERROR_TAREA))
			{
				string_append(&mensaje,"JOB: ");
				string_append(&mensaje,string_itoa(registro->job));
				string_append(&mensaje,"\tMASTER ID: ");
				string_append(&mensaje,string_itoa(registro->master));
				switch(registro->etapa)
				{
				case 1:
					string_append(&mensaje,"\tETAPA TRANSFORMACION");
					break;
				case 2:
					string_append(&mensaje,"\tETAPA REDUCCION LOCAL");
					break;
				case 3:
					string_append(&mensaje,"\tETAPA REDUCCION GLOBAL");
					break;
				}
				string_append(&mensaje,"\tARCHIVO TEMPORAL: ");
				string_append(&mensaje,registro->archivoTemp);
				string_append(&mensaje,"\ESTADO: ");
				if(registro->estado==COMPLETADO)
					string_append(&mensaje,"COMPLETADO");
				else
					string_append(&mensaje,"ERROR EN LA OPERACION");
			}
		log_info(logger,mensaje);
	}}
	//log_destroy(logger);
	exit(0);
}

void crearYAMALogger(t_log *logger) {
	char *pathLogger = string_new();
	char* temporal = string_new();
	char* sTime = temporal_get_string_time();
	char cwd[1024];
	string_append(&pathLogger, getcwd(cwd, sizeof(cwd)));
	string_append(&temporal, "/logs/YAMALogs");
	string_append(&temporal, sTime);
	string_append(&temporal,".log");
	string_append(&pathLogger, temporal);

	char *logYAMAFileName = strdup(temporal);
	free(sTime);
	free(temporal);
	free(logYAMAFileName);
	free(pathLogger);
	logYAMAFileName = NULL;
}
