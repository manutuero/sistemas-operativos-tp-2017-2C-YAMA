/*
 * funcionesMaster.c
 *
 *  Created on: 11/9/2017
 *      Author: utnso
 */


#include "funcionesMaster.h"
//#include "../../fileSystem/src/utils/utils.h"

int chequearParametros(char *transformador,char *reductor,char *archivoAprocesar,char *direccionDeResultado){


	char * comienzo ="yamafs:/";
		if(string_starts_with(archivoAprocesar,comienzo)<=0){
			printf("Parametro archivo a procesar invalido.: %s \n",archivoAprocesar);
			return 0;
		}
		if(string_starts_with(direccionDeResultado,comienzo)<=0){
			printf("La direccion de guardado de resultado es invalida: %s \n",direccionDeResultado);
			return 0;
		}

		if(!file_exists(transformador)){
			printf("El programa transformador no se encuentra en : %s  \n",transformador);
			return 0;
		}
		if(!file_exists(reductor)){
			printf("El programa reductor no se encuentra en : %s  \n",reductor);
			return 0;
			}


	return 1;

}
//Chequea existencia de archivo en linux
int file_exists (char * fileName)
{
   struct stat buf;
   int i = stat ( fileName, &buf );
     /* File found*/
     if ( i == 0 )
     {
       return 1;
     }
     return 0;

}

void crearListas(){
	listaTransformaciones = list_create();
	listaRedLocales = list_create();
	listaRedGloblales = list_create();
}

void masterEscuchando(int* socketMaster) {

	struct sockaddr_in dir;
	//configurarAddr(&dir);
	dir.sin_family = AF_INET;
	dir.sin_port = htons(24000);
	dir.sin_addr.s_addr = INADDR_ANY;
	memset(&(dir.sin_zero), '\0', 8);

	*socketMaster = socket(AF_INET, SOCK_STREAM, 0);
	// Olvidémonos del error "Address already in use" [La dirección ya se está usando]
	int opt = 1;
	if (setsockopt(*socketMaster, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int))
			== -1) {
		perror("setsockopt");
		exit(1);
	}
	if (bind(*socketMaster, (struct sockaddr*) &dir, sizeof(struct sockaddr))
			!= 0) {
		perror("fallo el bind");
		exit(1);
	}
	listen(*socketMaster, 100);
}

//FUNCION CON EL SELECT PARA LOS WORKERS
//TODO se va a cambiar por hilos mas adelante
void iniciarMaster(char* rutaTransformador,char* rutaReductor,char* archivoAprocesar,char* direccionDeResultado){
	//struct sockaddr_in dir;
	int socketMaster;
	int nuevoSocket , max_clients = 5 , i ;
	int maxPuerto;
	fd_set readfds, auxRead;
	int socketYama;

	crearListas();



	socketYama = conectarseAYama(6670,"127.0.0.1");

	mandarRutaArchivoAYama(socketYama, archivoAprocesar);


	recibirPlanificacionDeYama(socketYama);

	masterEscuchando(&socketMaster);

	//inicializar todos los socketCliente
	    for (i = 0; i < max_clients; i++)
	    {
	        //socketCliente[i] = 0;
	    }

	/********************************************************/

	struct sockaddr_in direccionCliente;
	int tamanioDir;

	char *buffer;
	int bytesRecibidos;

	FD_ZERO(&readfds);
	FD_ZERO(&auxRead);
	FD_SET(socketMaster, &auxRead);

	maxPuerto = socketMaster;
	tamanioDir = sizeof(struct sockaddr_in);

	//-----------------------

	enviarTransformacionAWorkers(rutaTransformador, rutaReductor); //FALTA HACER ESTO



	/*
	//Entro al select de conexiones
	printf("Esperando conexiones\n");
	while(1){
		readfds = auxRead;
		if (select(maxPuerto+1, &readfds, NULL, NULL, NULL) == -1) {
					perror("select");
					exit(1);
				}

		for ( i = 0 ; i <= maxPuerto ; i++)
		{
		    if(FD_ISSET(i, &readfds)){
		      if(i == socketMaster){

				if((nuevoSocket = accept(socketMaster, (void*)&direccionCliente, &tamanioDir)) <= 0) perror("accept");
				else{
					//Le envia el archivo apenas se conecta con un puerto
					printf("Entro una conexion por el puerto %d\n", nuevoSocket);
					FD_SET(nuevoSocket,&auxRead);
*/
					//TODO
					/*-------Envio archivo-------------------------
					 *Actualmente manda el primer parametro, en este caso el tranformador.
					 * Seguramente va a haber que cambiarlo
					 */
	/*
					enviarArchivo(nuevoSocket, buffer, rutaTransformador);
					if(nuevoSocket > maxPuerto)
						maxPuerto = nuevoSocket;
				}
		      }
		      else{
		    	char*  buffer = malloc(1001);
				bytesRecibidos = recibirPorSocket(i,buffer,1000);
				if(bytesRecibidos < 0){
					perror("Error");
					free(buffer);
					//break;
					exit(1);
				}
				if(bytesRecibidos == 0){
					//printf("Se desconecto del fileSystem el socket %d", i);
					 FD_CLR(i, &readfds);
					//free(buffer);
					 shutdown(i, 2);
				}else{
					 buffer[bytesRecibidos] = '\0';
					 printf("Socket: %d -- BytesRecibidos: %d -- Buffer recibido : %s\n",i, bytesRecibidos , buffer);
						free(buffer);
				}
		      }
		    }
		}
		*/


}


int conectarseAYama(int puerto,char* ip){
	struct sockaddr_in direccionYama;

	direccionYama.sin_family = AF_INET;
	direccionYama.sin_port = htons(puerto);
	direccionYama.sin_addr.s_addr = inet_addr(ip);
	//memset(&(direccionYama.sin_zero), '\0', 8);


	int yama;

	yama = socket(AF_INET, SOCK_STREAM, 0);

	if(connect(yama, (struct sockaddr *)&direccionYama, sizeof(struct sockaddr)) != 0){
			perror("fallo la conexion a YAMA");
			exit(1);
		}


	printf("se conecto a YAMA\n");

	return yama;
}

void mandarRutaArchivoAYama(int socketYama, char* archivoAprocesar){
	t_rutaArchivo ruta;
	t_header header;

	ruta.tamanio = strlen(archivoAprocesar)+1;
	ruta.ruta = archivoAprocesar;
	//int tamanioMensaje = header.tamanio + sizeof(header);

	void* buffer;
	buffer = serializarRutaArchivo(&header,&ruta); //esta en utils ya que lo voy a usar para Yama-fs
	int tamanioMensaje = header.tamanioPayload + sizeof(header);
	enviarPorSocket(socketYama,buffer,tamanioMensaje);
	free(buffer);
}

void recibirPlanificacionDeYama(int socketYama){

	void* buffer;

	t_header header;

	recibirHeader(socketYama, &header);

	if(header.id != 5) exit(0);

	buffer = malloc(header.tamanioPayload);

	recibirPorSocket(socketYama, buffer, header.tamanioPayload);

	deserializarPlanificacion(buffer);


	int i;
	printf("archivos de transformacion:\n");
	for(i=0;i<list_size(listaTransformaciones);i++){
		printf("en lista: %s\n", ((t_transformacionMaster*)list_get(listaTransformaciones,i))->ip);
		printf("en lista: %s\n", ((t_transformacionMaster*)list_get(listaTransformaciones,i))->archivoTransformacion);

	}

	printf("archivos de reduccion local:\n");
		for(i=0;i<list_size(listaRedLocales);i++){
			printf("en lista: %s\n", ((t_reduccionLocalMaster*)list_get(listaRedLocales,i))->archivoRedLocal);

		}

	printf("archivos de reduccion global:\n");
		for(i=0;i<list_size(listaRedGloblales);i++){
			printf("en lista: %s\n", ((t_reduccionGlobalMaster*)list_get(listaRedGloblales,i))->archivoRedGlobal);

	}
}

void deserializarPlanificacion(void* buffer){

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

	printf("cantTransformaciones: %d  cantRedLocal: %d cantRedGlobal: %d\n",cantTransformaciones,cantRedLocal,cantRedGlobal);

	t_transformacionMaster transformaciones[cantTransformaciones];
	t_reduccionLocalMaster reducciones[cantRedLocal];
	t_reduccionGlobalMaster reduccionesGlobales[cantRedGlobal];



	deserializarTransformaciones(cantTransformaciones, buffer, &desplazamiento);

	deserializarReduccionesLocales(cantRedLocal, buffer, &desplazamiento);

	deserializarReduccionesGlobales(cantRedGlobal, buffer, &desplazamiento);

	printf("archivos de transformacion:\n");
	for(i=0;i<list_size(listaTransformaciones);i++){
		printf("en lista: %s\n", ((t_transformacionMaster*)list_get(listaTransformaciones,i))->ip);
		printf("en lista: %s\n", ((t_transformacionMaster*)list_get(listaTransformaciones,i))->archivoTransformacion);

	}

	printf("archivos de reduccion local:\n");
		for(i=0;i<list_size(listaRedLocales);i++){
			printf("en lista: %s\n", ((t_reduccionLocalMaster*)list_get(listaRedLocales,i))->archivoRedLocal);

		}

	printf("archivos de reduccion global:\n");
		for(i=0;i<list_size(listaRedGloblales);i++){
			printf("en lista: %s\n", ((t_reduccionGlobalMaster*)list_get(listaRedGloblales,i))->archivoRedGlobal);

	}
}


void deserializarTransformaciones(int cantTransformaciones, void* buffer, int* desplazamiento){
	int i;
	t_transformacionMaster* transformaciones;

	for(i=0;i<cantTransformaciones;i++){
		transformaciones = malloc(sizeof(t_transformacionMaster));
		memcpy(&transformaciones->idNodo,buffer+*desplazamiento,  sizeof(transformaciones->idNodo));
		*desplazamiento+=sizeof(transformaciones->idNodo);
		memcpy(&transformaciones->nroBloqueNodo, buffer+*desplazamiento, sizeof(transformaciones->nroBloqueNodo));
		*desplazamiento+=sizeof(transformaciones->nroBloqueNodo);
		memcpy(&transformaciones->bytesOcupados,buffer+*desplazamiento,  sizeof(transformaciones->bytesOcupados));
		*desplazamiento+=sizeof(transformaciones->bytesOcupados);
		memcpy(&transformaciones->puerto, buffer+*desplazamiento, sizeof(transformaciones->puerto));
		*desplazamiento+=sizeof(transformaciones->puerto);
		memcpy(&transformaciones->largoIp, buffer+*desplazamiento, sizeof(transformaciones->largoIp));
		*desplazamiento+=sizeof(transformaciones->largoIp);
		transformaciones->ip = malloc(transformaciones->largoIp);
		memcpy(transformaciones->ip, buffer+*desplazamiento, transformaciones->largoIp);
		*desplazamiento+=transformaciones->largoIp;
		memcpy(&transformaciones->largoArchivo, buffer+*desplazamiento, sizeof(transformaciones->largoArchivo));
		*desplazamiento+=sizeof(transformaciones->largoArchivo);

		transformaciones->archivoTransformacion = malloc(transformaciones->largoArchivo);
		memcpy(transformaciones->archivoTransformacion,buffer+*desplazamiento, transformaciones->largoArchivo);
		*desplazamiento+=transformaciones->largoArchivo;
		//free(transformaciones->ip);
		//free(transformaciones->archivoTransformacion);

		list_add(listaTransformaciones, transformaciones);
	}
}


void deserializarReduccionesLocales(int cantRedLocales, void* buffer, int* desplazamiento){
	int i;

	t_reduccionLocalMaster* reducciones;

	for(i=0;i<cantRedLocales;i++){
		reducciones = malloc(sizeof(t_reduccionLocalMaster));
		memcpy(&reducciones->idNodo,buffer+*desplazamiento,  sizeof(reducciones->idNodo));
		*desplazamiento+=sizeof(reducciones[i].idNodo);
		memcpy(&reducciones->puerto, buffer+*desplazamiento, sizeof(reducciones->puerto));
		*desplazamiento+=sizeof(reducciones->puerto);

		memcpy(&reducciones->largoIp, buffer+*desplazamiento, sizeof(reducciones->largoIp));
		*desplazamiento+=sizeof(reducciones->largoIp);
		reducciones->ip = malloc(reducciones->largoIp);
		memcpy(reducciones->ip, buffer+*desplazamiento, reducciones->largoIp);
		*desplazamiento+=reducciones->largoIp;

		memcpy(&reducciones->largoArchivoTransformacion, buffer+*desplazamiento, sizeof(reducciones->largoArchivoTransformacion));
		*desplazamiento+=sizeof(reducciones->largoArchivoTransformacion);
		reducciones->archivoTransformacion = malloc(reducciones->largoArchivoTransformacion);
		memcpy(reducciones->archivoTransformacion, buffer+*desplazamiento, reducciones->largoArchivoTransformacion);
		*desplazamiento+=reducciones->largoArchivoTransformacion;

		memcpy(&reducciones->largoArchivoRedLocal, buffer+*desplazamiento, sizeof(reducciones->largoIp));
		*desplazamiento+=sizeof(reducciones->largoArchivoRedLocal);
		reducciones->archivoRedLocal = malloc(reducciones->largoArchivoRedLocal);
		memcpy(reducciones->archivoRedLocal,buffer+*desplazamiento, reducciones->largoArchivoRedLocal);
		*desplazamiento+=reducciones->largoArchivoRedLocal;

		list_add(listaRedLocales, reducciones);
		//free(transformaciones->ip);
		//free(transformaciones->archivoTransformacion);
	}
}


void deserializarReduccionesGlobales(int cantRedGlobales, void* buffer, int* desplazamiento){
	int i;

	t_reduccionGlobalMaster* reduccionesGlobales;

	for(i=0;i<cantRedGlobales;i++){
		reduccionesGlobales = malloc(sizeof(t_reduccionGlobalMaster));
		memcpy(&reduccionesGlobales->idNodo,buffer+*desplazamiento,  sizeof(reduccionesGlobales->idNodo));
		*desplazamiento+=sizeof(reduccionesGlobales->idNodo);
		memcpy(&reduccionesGlobales->encargado, buffer+*desplazamiento, sizeof(reduccionesGlobales->encargado));
		*desplazamiento+=sizeof(reduccionesGlobales->encargado);
		memcpy(&reduccionesGlobales->puerto, buffer+*desplazamiento, sizeof(reduccionesGlobales->puerto));
		*desplazamiento+=sizeof(reduccionesGlobales->puerto);
		memcpy(&reduccionesGlobales->largoIp, buffer+*desplazamiento, sizeof(reduccionesGlobales->largoIp));
		*desplazamiento+=sizeof(reduccionesGlobales->largoIp);
		reduccionesGlobales->ip = malloc(reduccionesGlobales->largoIp);
		memcpy(reduccionesGlobales->ip, buffer+*desplazamiento, reduccionesGlobales->largoIp);
		*desplazamiento+=reduccionesGlobales->largoIp;

		memcpy(&reduccionesGlobales->largoArchivoRedLocal, buffer+*desplazamiento, sizeof(reduccionesGlobales->largoIp));
		*desplazamiento+=sizeof(reduccionesGlobales->largoIp);
		reduccionesGlobales->archivoRedLocal = malloc(reduccionesGlobales->largoArchivoRedLocal);
		memcpy(reduccionesGlobales->archivoRedLocal,buffer+*desplazamiento, reduccionesGlobales->largoArchivoRedLocal);
		*desplazamiento+=reduccionesGlobales->largoArchivoRedLocal;

		memcpy(&reduccionesGlobales->largoArchivoRedGlobal, buffer+*desplazamiento, sizeof(reduccionesGlobales->largoArchivoRedGlobal));
		*desplazamiento+=sizeof(reduccionesGlobales->largoArchivoRedGlobal);
		reduccionesGlobales->archivoRedGlobal = malloc(reduccionesGlobales->largoArchivoRedGlobal);
		memcpy(reduccionesGlobales->archivoRedGlobal,buffer+*desplazamiento, reduccionesGlobales->largoArchivoRedGlobal);
		*desplazamiento+=reduccionesGlobales->largoArchivoRedGlobal;

		list_add(listaRedGloblales, reduccionesGlobales);
		//free(transformaciones->ip);
		//free(transformaciones->archivoTransformacion);
	}
}

void enviarTransformacionAWorkers(char* rutaTransformador, char* rutaReductor){

	int i;
	t_transformacionMaster transformacion;


	printf("enviar etapa de transformacion: \n");
	for(i=0;i<list_size(listaTransformaciones);i++){
		//transformacion = malloc(sizeof(t_transformacionMaster));
		transformacion = *(t_transformacionMaster*) list_get(listaTransformaciones, i);

		pthread_t hiloConexionesWorker;
		pthread_create(&hiloConexionesWorker, NULL, (void*)hiloConexionWorker, &transformacion);


		pthread_join(hiloConexionesWorker, NULL);
		//Esta parte es la que podria ir en el pthread
		printf("en lista: %s\n", ((t_transformacionMaster*)list_get(listaTransformaciones,i))->ip);

		printf("ip: %s, tamanio = %d, temporal: %s\n", transformacion.ip, transformacion.largoIp, transformacion.archivoTransformacion);
		//free(transformacion);




	}
	while(1){

	}
}

void hiloConexionWorker(t_transformacionMaster* transformacion){
	t_transformacionWorker* worker;
	void* buffer, *bufferMensaje;
	int largoBuffer, tamanioMensaje, desplazamiento = 0;

	worker = malloc(sizeof(t_transformacionWorker));
//	worker->socketWorker = socketWorker;
	worker->bloqueATransformar = transformacion->nroBloqueNodo;
	worker->bytesOcupados = transformacion->bytesOcupados;
	worker->largoArchivoTransformador = transformacion->largoArchivo;
	worker->rutaArchivoTemporal = malloc(transformacion->largoArchivo);
	strcpy(worker->rutaArchivoTemporal,transformacion->archivoTransformacion);
	worker->largoArchivoTransformador = devolverTamanioArchivo();
	worker->archivoTransformador = malloc(worker->largoArchivoTransformador);
	worker->etapa = 1; //DEFINIR MACRO TRANSFORMACION

	printf("%d\n", worker->largoArchivoTransformador);

	int socketWorker = conectarseAWorker(transformacion->puerto, transformacion->ip);


	buffer = serializarTransformacionWorker(worker, &largoBuffer);
	tamanioMensaje = largoBuffer+sizeof(t_header);
	bufferMensaje = malloc(tamanioMensaje);
	t_header header;
	header.id = 3;
	header.tamanioPayload = largoBuffer;

	memcpy(bufferMensaje, &header.id, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(bufferMensaje+desplazamiento, &header.tamanioPayload, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(bufferMensaje+desplazamiento, buffer, header.tamanioPayload);

	enviarPorSocket(socketWorker, bufferMensaje, tamanioMensaje);
	free(buffer);
	free(bufferMensaje);
	printf("enviado a worker\n");
}

int conectarseAWorker(int puerto, char* ip){

	struct sockaddr_in direccionWorker;

	direccionWorker.sin_family = AF_INET;
	direccionWorker.sin_port = htons(puerto);
	direccionWorker.sin_addr.s_addr = inet_addr(ip);
	//memset(&(direccionYama.sin_zero), '\0', 8);


	int socketWorker;

	socketWorker = socket(AF_INET, SOCK_STREAM, 0);

	if(connect(socketWorker, (struct sockaddr *)&direccionWorker, sizeof(struct sockaddr)) != 0){
			perror("fallo la conexion al worker");
			exit(1);
		}

	return socketWorker;

}

void* serializarTransformacionWorker(t_transformacionWorker* worker, int* largoBuffer){
	void* buffer;
	int desplazamiento = 0;
	int tamanioBuffer = worker->largoRutaArchivo + worker->largoArchivoTransformador + 5*sizeof(uint32_t);
	buffer = malloc(tamanioBuffer);
	memcpy(buffer+desplazamiento, &worker->bloqueATransformar, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, &worker->bytesOcupados, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, &worker->etapa, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, &worker->largoRutaArchivo, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(buffer+desplazamiento, worker->rutaArchivoTemporal, worker->largoRutaArchivo);
	desplazamiento += worker->largoRutaArchivo;

	memcpy(buffer+desplazamiento, &worker->largoArchivoTransformador, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, worker->archivoTransformador, worker->largoArchivoTransformador);
	desplazamiento += worker->largoArchivoTransformador;

	*largoBuffer = desplazamiento;

	return buffer;
}

int devolverTamanioArchivo(){
	printf("%s\n", archivoAprocesar);
	int file = open(archivoAprocesar, O_RDONLY);
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

	return tam;
}
