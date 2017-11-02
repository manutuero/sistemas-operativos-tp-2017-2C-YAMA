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

void destruirListas(){
	int i;
	t_transformacionMaster* tmaster;
	for(i=0;i<list_size(listaTransformaciones);i++){
			free(((t_transformacionMaster*)list_get(listaTransformaciones,i))->ip);
			free(((t_transformacionMaster*)list_get(listaTransformaciones,i))->archivoTransformacion);
	}

	for(i=0;i<list_size(listaRedLocales);i++){
			free(((t_reduccionLocalMaster*)list_get(listaRedLocales,i))->ip);
			free(((t_reduccionLocalMaster*)list_get(listaRedLocales,i))->archivoTransformacion);
			free(((t_reduccionLocalMaster*)list_get(listaRedLocales,i))->archivoRedLocal);

	}
	for(i=0;i<list_size(listaRedGloblales);i++){
			free(((t_reduccionGlobalMaster*)list_get(listaRedGloblales,i))->ip);
			free(((t_reduccionGlobalMaster*)list_get(listaRedGloblales,i))->archivoRedLocal);
			free(((t_reduccionGlobalMaster*)list_get(listaRedGloblales,i))->archivoRedGlobal);

	}

	list_destroy_and_destroy_elements(listaTransformaciones,free);
	list_destroy_and_destroy_elements(listaRedLocales,free);
	list_destroy_and_destroy_elements(listaRedGloblales,free);
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

//TODO se va a cambiar por hilos mas adelante
void iniciarMaster(char* rutaTransformador,char* rutaReductor,char* archivoAprocesar,char* direccionDeResultado){

	//int socketYama;

	crearListas();

	socketYama = conectarseAYama(6670,"127.0.0.1");

	mandarRutaArchivoAYama(socketYama, archivoAprocesar);


	recibirPlanificacionDeYama(socketYama);

	//masterEscuchando(&socketMaster);

	operarEtapas();



	destruirListas();
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


	printf("se conecto a YAMA en el socket %d\n",yama);

	return yama;
}

void mandarRutaArchivoAYama(int socketYama, char* archivoAprocesar){
	t_rutaArchivo ruta;
	t_header header;

	ruta.tamanio = strlen(archivoAprocesar);
	ruta.ruta = archivoAprocesar;
	//ruta.ruta[ruta.tamanio] = '\0';
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
	free(buffer);
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
		transformaciones->ip = malloc(transformaciones->largoIp+1);
		memcpy(transformaciones->ip, buffer+*desplazamiento, transformaciones->largoIp);
		*desplazamiento+=transformaciones->largoIp;
		transformaciones->ip[transformaciones->largoIp]='\0';
		memcpy(&transformaciones->largoArchivo, buffer+*desplazamiento, sizeof(transformaciones->largoArchivo));
		*desplazamiento+=sizeof(transformaciones->largoArchivo);

		transformaciones->archivoTransformacion = malloc(transformaciones->largoArchivo+1);
		memcpy(transformaciones->archivoTransformacion,buffer+*desplazamiento, transformaciones->largoArchivo);
		*desplazamiento+=transformaciones->largoArchivo;
		transformaciones->archivoTransformacion[transformaciones->largoArchivo]='\0';
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
		reducciones->ip = malloc(reducciones->largoIp+1);
		memcpy(reducciones->ip, buffer+*desplazamiento, reducciones->largoIp);
		*desplazamiento+=reducciones->largoIp;
		reducciones->ip[reducciones->largoIp]='\0';

		memcpy(&reducciones->largoArchivoTransformacion, buffer+*desplazamiento, sizeof(reducciones->largoArchivoTransformacion));
		*desplazamiento+=sizeof(reducciones->largoArchivoTransformacion);
		reducciones->archivoTransformacion = malloc(reducciones->largoArchivoTransformacion+1);
		memcpy(reducciones->archivoTransformacion, buffer+*desplazamiento, reducciones->largoArchivoTransformacion);
		*desplazamiento+=reducciones->largoArchivoTransformacion;
		reducciones->archivoTransformacion[reducciones->largoArchivoTransformacion]='\0';

		memcpy(&reducciones->largoArchivoRedLocal, buffer+*desplazamiento, sizeof(reducciones->largoIp));
		*desplazamiento+=sizeof(reducciones->largoArchivoRedLocal);
		reducciones->archivoRedLocal = malloc(reducciones->largoArchivoRedLocal+1);
		memcpy(reducciones->archivoRedLocal,buffer+*desplazamiento, reducciones->largoArchivoRedLocal);
		*desplazamiento+=reducciones->largoArchivoRedLocal;
		reducciones->archivoRedLocal[reducciones->largoArchivoRedLocal]='\0';

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
		reduccionesGlobales->ip = malloc(reduccionesGlobales->largoIp+1);
		memcpy(reduccionesGlobales->ip, buffer+*desplazamiento, reduccionesGlobales->largoIp);
		*desplazamiento+=reduccionesGlobales->largoIp;
		reduccionesGlobales->ip[reduccionesGlobales->largoIp]='\0';

		memcpy(&reduccionesGlobales->largoArchivoRedLocal, buffer+*desplazamiento, sizeof(reduccionesGlobales->largoIp));
		*desplazamiento+=sizeof(reduccionesGlobales->largoIp);
		reduccionesGlobales->archivoRedLocal = malloc(reduccionesGlobales->largoArchivoRedLocal+1);
		memcpy(reduccionesGlobales->archivoRedLocal,buffer+*desplazamiento, reduccionesGlobales->largoArchivoRedLocal);
		*desplazamiento+=reduccionesGlobales->largoArchivoRedLocal;
		reduccionesGlobales->archivoRedLocal[reduccionesGlobales->largoArchivoRedLocal]='\0';
		memcpy(&reduccionesGlobales->largoArchivoRedGlobal, buffer+*desplazamiento, sizeof(reduccionesGlobales->largoArchivoRedGlobal));
		*desplazamiento+=sizeof(reduccionesGlobales->largoArchivoRedGlobal);
		reduccionesGlobales->archivoRedGlobal = malloc(reduccionesGlobales->largoArchivoRedGlobal+1);
		memcpy(reduccionesGlobales->archivoRedGlobal,buffer+*desplazamiento, reduccionesGlobales->largoArchivoRedGlobal);
		*desplazamiento+=reduccionesGlobales->largoArchivoRedGlobal;
		reduccionesGlobales->archivoRedGlobal[reduccionesGlobales->largoArchivoRedGlobal]='\0';

		list_add(listaRedGloblales, reduccionesGlobales);
		//free(transformaciones->ip);
		//free(transformaciones->archivoTransformacion);
	}
}

void operarEtapas(){

	int i,j;
	int transformaciones = list_size(listaTransformaciones);
	int redLocales = list_size(listaRedLocales);
	int redGlobales = list_size(listaRedGloblales);

	//t_transformacionesNodo nodosTransformacion[redGlobales];
	nodosTransformacion = malloc(sizeof(t_transformacionesNodo)*transformaciones);

	for(i=0;i<transformaciones;i++){
		 nodosTransformacion[i].idNodo = 0;
		 nodosTransformacion[i].cantidadTransformaciones = 0;
		}

	t_transformacionMaster* tmaster;

	for(i=0;i<transformaciones;i++){
		 tmaster = list_get(listaTransformaciones, i);
		 for(j=0;j<redGlobales;j++){
			 if(nodosTransformacion[j].idNodo==tmaster->idNodo)
				 nodosTransformacion[j].cantidadTransformaciones++;
			 if(nodosTransformacion[j].idNodo==0){
				nodosTransformacion[j].idNodo = tmaster->idNodo;
				nodosTransformacion[j].cantidadTransformaciones = 1;
				j=redGlobales;
			 }
		}
	}

	enviarTransformacionAWorkers(transformador, reductor);

	for(i=0;i<list_size(listaRedGloblales);i++){
			if(nodosTransformacion[i].cantidadTransformaciones == 0){
				printf("termino todas las transformaciones del nodo %d\n",nodosTransformacion[i].idNodo);

				pthread_t hiloConexionReduccionWorker;
				pthread_create(&hiloConexionReduccionWorker, NULL, (void*)enviarRedLocalesAWorker,
						(t_reduccionGlobalMaster*)list_get(listaRedGloblales,i));


				pthread_join(hiloConexionReduccionWorker, NULL);
						/*
				enviarReduccionLocalAWorker(nodosTransformacion[i].idNodo,
						((t_reduccionGlobalMaster*)list_get(listaRedGloblales,i)));
		*/
		}
	}

	free(nodosTransformacion);
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

	/*
	while(1){

		for(i=0;i<list_size(listaRedGloblales);i++){
			if(nodosTransformacion[i].cantidadTransformaciones == 0){
				//sprintf("termino todas las transformaciones del nodo %d\n",nodosTransformacion[i].idNodo);
			}
		}


	}
*/
}

void enviarRedLocalesAWorker(t_reduccionGlobalMaster* nodoReduccion){
	int i;
	t_reduccionLocalMaster redLocalMaster;
	t_redLocalesWorker redLocalWorker;
	t_temporalesTransformacionWorker temporales;

	redLocalWorker.etapa = 2;
	redLocalWorker.largoArchivoReductorLocal = strlen(nodoReduccion->archivoRedLocal);
	redLocalWorker.archivoReductorLocal = nodoReduccion->archivoRedLocal;
	redLocalWorker.temporalesTranformacion = list_create();

	for(i=0;i<list_size(listaRedLocales);i++){
		redLocalMaster = *(t_reduccionLocalMaster*) list_get(listaRedLocales,i);
		if(redLocalMaster.idNodo == nodoReduccion->idNodo){
			temporales.largoRutaTemporalTransformacion = redLocalMaster.largoArchivoTransformacion;
			temporales.rutaTemporalTransformacion = malloc(temporales.largoRutaTemporalTransformacion);
			strcpy(temporales.rutaTemporalTransformacion,redLocalMaster.archivoTransformacion);
			list_add(redLocalWorker.temporalesTranformacion,&temporales);
		}
	}
	redLocalWorker.cantidadTransformaciones = list_size(redLocalWorker.temporalesTranformacion);

	int socketWorker = conectarseAWorker(nodoReduccion->puerto, nodoReduccion->ip);

	if(socketWorker == -1){
			printf("envio desconexion del nodo a yama en el socket %d\n",socketYama);
		}

	else {
			int tamanioMensaje, largoBuffer, desplazamiento = 0;
			void* bufferMensaje;
			void* buffer;
			buffer = serializarReduccionLocalWorker(&redLocalWorker, &largoBuffer);
			tamanioMensaje = largoBuffer+sizeof(t_header);
			bufferMensaje = malloc(tamanioMensaje);
			t_header header;
			header.id = 31;
			header.tamanioPayload = largoBuffer;

			memcpy(bufferMensaje, &header.id, sizeof(uint32_t));
			desplazamiento += sizeof(uint32_t);
			memcpy(bufferMensaje+desplazamiento, &header.tamanioPayload, sizeof(uint32_t));
			desplazamiento += sizeof(uint32_t);
			memcpy(bufferMensaje+desplazamiento, buffer, header.tamanioPayload);

			enviarPorSocket(socketWorker, bufferMensaje, tamanioMensaje);
			free(buffer);
			free(bufferMensaje);
			printf("enviado a worker la reduccion local\n");

	}

	//llenar la estructura
	//buscarlo en la estructura nodos para el socket
}

void avisarAYama(t_transformacionMaster* transformacion) {
	int desplazamiento;
	t_header headerResp;
	headerResp.id = 20;
	headerResp.tamanioPayload = transformacion->largoArchivo+1;
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

	int bytesRecibidos = enviarPorSocket(socketYama, buffer, desplazamiento);
	free(buffer);
}

			// * Hilo conexion con cada worker   * ///

void hiloConexionWorker(t_transformacionMaster* transformacion){
	t_transformacionWorker* worker;
	void* buffer, *bufferMensaje;
	int largoBuffer, tamanioMensaje, desplazamiento = 0;

	worker = malloc(sizeof(t_transformacionWorker));
//	worker->socketWorker = socketWorker;
	worker->bloqueATransformar = transformacion->nroBloqueNodo;
	//worker->bytesOcupados = transformacion->bytesOcupados;
	worker->largoRutaArchivo = transformacion->largoArchivo+1;
	worker->rutaArchivoTemporal = malloc(worker->largoRutaArchivo);
	strcpy(worker->rutaArchivoTemporal,transformacion->archivoTransformacion);
	worker->largoArchivoTransformador = devolverTamanioArchivo(transformador);
	//worker->archivoTransformador = malloc(worker->largoArchivoTransformador);
	worker->archivoTransformador = obtenerContenidoArchivo(transformador);
	worker->etapa = 1; //DEFINIR MACRO TRANSFORMACION

	printf("%d\n", worker->largoArchivoTransformador);
	printf("%s\n", worker->archivoTransformador);

	transformacion->puerto = 24000;
	int socketWorker = conectarseAWorker(transformacion->puerto, transformacion->ip);

	if(socketWorker == -1){
		printf("envio desconexion del nodo a yama en el socket %d\n",socketYama);
	}
	else {
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



		if(respuestaTransformacion(socketWorker) == 10){
			printf("transformacion OK\n");
			//entre mutex
			disminuirTransformacionesDeNodo(transformacion->idNodo);

			avisarAYama(transformacion);

		}
	}
	free(worker->rutaArchivoTemporal);
	free(worker->archivoTransformador);
	free(worker);
}

void disminuirTransformacionesDeNodo(int nodo){
	int i;
	for(i=0;i<list_size(listaRedGloblales);i++){
		if(nodosTransformacion[i].idNodo==nodo)
			nodosTransformacion[i].cantidadTransformaciones--;
	}
}

int conectarseAWorker(int puerto, char* ip){

	struct sockaddr_in direccionWorker;

	direccionWorker.sin_family = AF_INET;
	direccionWorker.sin_port = htons(puerto);
	direccionWorker.sin_addr.s_addr = inet_addr(ip);
	//memset(&(direccionYama.sin_zero), '\0', 8);
	direccionWorker.sin_port = htons(24000);

	int socketWorker;

	socketWorker = socket(AF_INET, SOCK_STREAM, 0);

	if(connect(socketWorker, (struct sockaddr *)&direccionWorker, sizeof(struct sockaddr)) != 0){
			perror("fallo la conexion al worker");
			printf("El nodo no se encontraba levantado.\n");
			return -1;
		}

	return socketWorker;

}

void* serializarTransformacionWorker(t_transformacionWorker* worker, int* largoBuffer){
	void* buffer;
	int desplazamiento = 0;
	int tamanioBuffer = worker->largoRutaArchivo + worker->largoArchivoTransformador + 4*sizeof(uint32_t);
	buffer = malloc(tamanioBuffer);
	memcpy(buffer+desplazamiento, &worker->bloqueATransformar, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	//memcpy(buffer+desplazamiento, &worker->bytesOcupados, sizeof(uint32_t));
	//desplazamiento += sizeof(uint32_t);
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

void* serializarReduccionLocalWorker(t_redLocalesWorker* redLocalWorker, int* largoBuffer){
	int i, desplazamiento = 0, tamanioBuffer=0;
	void* buffer;
	t_temporalesTransformacionWorker temporales;

	tamanioBuffer = 3*sizeof(uint32_t) + redLocalWorker->largoArchivoReductorLocal;

	buffer = malloc(tamanioBuffer);

	//llenar el memcpy de redLocal

	memcpy(buffer+desplazamiento, &redLocalWorker->etapa, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(buffer+desplazamiento, &redLocalWorker->largoArchivoReductorLocal, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, redLocalWorker->archivoReductorLocal, redLocalWorker->largoArchivoReductorLocal);
	desplazamiento += redLocalWorker->largoArchivoReductorLocal;

	memcpy(buffer+desplazamiento, &redLocalWorker->cantidadTransformaciones, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	for(i=0;i<list_size(redLocalWorker->temporalesTranformacion);i++){
		temporales = *(t_temporalesTransformacionWorker*)(list_get(redLocalWorker->temporalesTranformacion,i));
		tamanioBuffer += temporales.largoRutaTemporalTransformacion + sizeof(uint32_t);
		buffer = realloc(buffer,tamanioBuffer);

		memcpy(buffer+desplazamiento, &temporales.largoRutaTemporalTransformacion, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(buffer+desplazamiento, temporales.rutaTemporalTransformacion, temporales.largoRutaTemporalTransformacion);
		desplazamiento += temporales.largoRutaTemporalTransformacion;
	}

	*largoBuffer = tamanioBuffer;
	return buffer;
}

int respuestaTransformacion(int socketWorker){

	char* buffer = malloc(sizeof(int));
	int respuesta;
	recibirPorSocket(socketWorker, &respuesta, sizeof(int));
	//respuesta = atoi(buffer);
	free(buffer);
	return respuesta;
}

int devolverTamanioArchivo(char* archivo){
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

char* obtenerContenidoArchivo(char* archivo){
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
		buffer[i]='\0';
		close(file);
		return buffer;
}
