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
	int bytesRecibidos, maxPuerto, i, nuevoSocket;
	FD_ZERO(&readfds);
	FD_ZERO(&auxRead);
	FD_SET(socketMasters, &auxRead);

	maxPuerto = socketMasters;
	inicializarWorkers(); //inicializa antes de entrar a la plani. sacar cuando este la conexion de nodos 
	int redLocales = 0;
	printf("escuchando masters\n");
	while (1) {

		readfds = auxRead;
		if (select(maxPuerto + 1, &readfds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(1);
		}

		for (i = 0; i <= maxPuerto; i++) {
			if (FD_ISSET(i, &readfds)) {
				if (i == socketMasters) {

					if ((nuevoSocket = accept(socketMasters,(void*) &direccionYama, &tamanioDir)) <= 0)
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
							printf("termino la trans del temporal %s\n",temporal);


							if(cambiarEstado(temporal,COMPLETADO))
								{
									headerResp.id = 13;
									headerResp.tamanioPayload=0;
									enviarPorSocket(i,&headerResp,0);
								}

							free(temporal);

							break;

						case 16:
						//	char* temporal = malloc(headerResp.tamanioPayload+1);
							temporal[headerResp.tamanioPayload] = '\0';
							bytesRecibidos = recibirPorSocket(i,temporal, headerResp.tamanioPayload);
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
							char *nombreTMP=NULL;
							buffer = malloc(headerResp.tamanioPayload);
							recibirPorSocket(i,buffer,headerResp.tamanioPayload);
							pedido = deserializarTresRutasArchivos(buffer,nombreTMP);
							t_tabla_estados* registro = encontrarRegistro(pedido->nombreArchivo);
							t_job* jobMaster = malloc(sizeof(t_job));
							jobMaster->job = registro->job;
							jobMaster->idMaster = registro->master;
							jobMaster->socketMaster = i;
							jobMaster->replanifica = 1;
							printf("repre2\n");
							rePrePlanificacion(nombreTMP,pedido->nombreArchivo,pedido->nombreArchivoGuardadoFinal, jobMaster);
							break;
						default:
							printf("Header ID erroneo");
						}
						/*
						if(headerResp.id == 12){

							char* temporal = malloc(headerResp.tamanioPayload+1);
							temporal[headerResp.tamanioPayload] = '\0';
							bytesRecibidos = recibirPorSocket(i,temporal, headerResp.tamanioPayload);
							printf("termino la trans del temporal %s\n",temporal);


							if(cambiarEstado(temporal,COMPLETADO))
							headerResp.id = 13;
							headerResp.tamanioPayload=0;
							enviarPorSocket(i,&headerResp,0);
							free(temporal);
					}
					if(headerResp.id == 16)
					{
						char* temporal = malloc(headerResp.tamanioPayload+1);
						temporal[headerResp.tamanioPayload] = '\0';
						bytesRecibidos = recibirPorSocket(i,temporal, headerResp.tamanioPayload);
						printf("termino la redlocal del temporal %s\n",temporal);
						free(temporal);
						redLocales++;
						if(redLocales%3==0){
							headerResp.id = 17;
							headerResp.tamanioPayload=0;
							//enviarPorSocket(i,&respuesta,sizeof(uint32_t));
							enviarPorSocket(i,&headerResp,0);
						}
					}
					if(headerResp.id == 20)
					{
						char* temporal = malloc(headerResp.tamanioPayload+1);
						temporal[headerResp.tamanioPayload] = '\0';
						bytesRecibidos = recibirPorSocket(i,temporal, headerResp.tamanioPayload);
						printf("termino la redglobal del temporal %s\n",temporal);
						uint32_t respuesta = 21;
						headerResp.id = 21;
						headerResp.tamanioPayload=0;
						enviarPorSocket(i,&headerResp,0);
						descargarWorkload(temporal);
						free(temporal);
						printf("%d",respuesta);
					}
					if(headerResp.id == 103){
							printf("repreplanifica\n");
							t_pedidoTransformacion* pedido = malloc(sizeof(t_pedidoTransformacion));
							char *nombreTMP=NULL;
							buffer = malloc(headerResp.tamanioPayload);
							recibirPorSocket(i,buffer,headerResp.tamanioPayload);
							pedido = deserializarTresRutasArchivos(buffer,nombreTMP);
							t_tabla_estados* registro = encontrarRegistro(pedido->nombreArchivo);
							t_job* jobMaster = malloc(sizeof(t_job));
							jobMaster->job = registro->job;
							jobMaster->idMaster = registro->master;
							jobMaster->socketMaster = i;
							jobMaster->replanifica = 1;
							printf("repre2\n");
							rePrePlanificacion(nombreTMP,pedido->nombreArchivo,pedido->nombreArchivoGuardadoFinal, jobMaster);

						}*/
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

	int desplazamiento = 0, bytesACopiar = 0, tamanioIp = 0;
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

	bytesACopiar = tamanioIp;
	infoNodo.IP = malloc(tamanioIp + 1);
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

t_pedidoTransformacion* deserializarTresRutasArchivos(void *buffer,char *nombreTMP)
{
	t_pedidoTransformacion *pedido;
	pedido = malloc (sizeof(t_pedidoTransformacion));
	uint32_t largo,desplazamiento=0;

	//Nombre temporal
	memcpy(&largo,buffer+desplazamiento,sizeof(uint32_t));
	nombreTMP = malloc(largo);
	desplazamiento+=sizeof(uint32_t);

	memcpy(nombreTMP,buffer+desplazamiento,largo);
	desplazamiento+=largo;

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

