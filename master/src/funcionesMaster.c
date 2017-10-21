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
void iniciarMaster(char* transformador,char* reductor,char* archivoAprocesar,char* direccionDeResultado){
	//struct sockaddr_in dir;
	int socketMaster;
	int nuevoSocket , max_clients = 5 , i ;
	int maxPuerto;
	fd_set readfds, auxRead;
	int socketYama;



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

					//TODO
					/*-------Envio archivo-------------------------
					 *Actualmente manda el primer parametro, en este caso el tranformador.
					 * Seguramente va a haber que cambiarlo
					 */
					enviarArchivo(nuevoSocket, buffer, transformador);
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
	}

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

	deserializarTransformaciones(transformaciones, cantTransformaciones, buffer, &desplazamiento);

	deserializarReduccionesLocales(reducciones, cantRedLocal, buffer, &desplazamiento);

	deserializarReduccionesGlobales(reduccionesGlobales, cantRedGlobal, buffer, &desplazamiento);

	printf("archivos de transformacion:\n");
	for(i=0;i<cantTransformaciones;i++){
		printf("%s\n",transformaciones[i].archivoTransformacion);
		//free(transformaciones[i].ip);
		//free(transformaciones[i].archivoTransformacion);
	}

	printf("archivos de reduccion local:\n");
	for(i=0;i<cantRedLocal;i++){
			printf("%s\n",reducciones[i].archivoRedLocal);
			//free(transformaciones[i].ip);
			//free(transformaciones[i].archivoTransformacion);
		}

	printf("archivo de reduccion global:\n");
	for(i=0;i<cantRedGlobal;i++){
			printf("%s\n",reduccionesGlobales[i].archivoRedGlobal);
			//free(transformaciones[i].ip);
			//free(transformaciones[i].archivoTransformacion);
	}
}


void deserializarTransformaciones(t_transformacionMaster* transformaciones, int cantTransformaciones, void* buffer, int* desplazamiento){
	int i;

	for(i=0;i<cantTransformaciones;i++){

		memcpy(&transformaciones[i].idNodo,buffer+*desplazamiento,  sizeof(transformaciones[i].idNodo));
		*desplazamiento+=sizeof(transformaciones[i].idNodo);
		memcpy(&transformaciones[i].nroBloqueNodo, buffer+*desplazamiento, sizeof(transformaciones[i].nroBloqueNodo));
		*desplazamiento+=sizeof(transformaciones[i].nroBloqueNodo);
		memcpy(&transformaciones[i].bytesOcupados,buffer+*desplazamiento,  sizeof(transformaciones[i].bytesOcupados));
		*desplazamiento+=sizeof(transformaciones[i].bytesOcupados);
		memcpy(&transformaciones[i].puerto, buffer+*desplazamiento, sizeof(transformaciones[i].puerto));
		*desplazamiento+=sizeof(transformaciones[i].puerto);
		memcpy(&transformaciones[i].largoIp, buffer+*desplazamiento, sizeof(transformaciones[i].largoIp));
		*desplazamiento+=sizeof(transformaciones[i].largoIp);
		transformaciones[i].ip = malloc(transformaciones[i].largoIp);
		memcpy(transformaciones[i].ip, buffer+*desplazamiento, transformaciones[i].largoIp);
		*desplazamiento+=transformaciones[i].largoIp;
		memcpy(&transformaciones[i].largoArchivo, buffer+*desplazamiento, sizeof(transformaciones[i].largoArchivo));
		*desplazamiento+=sizeof(transformaciones[i].largoArchivo);

		transformaciones[i].archivoTransformacion = malloc(transformaciones[i].largoArchivo);
		memcpy(transformaciones[i].archivoTransformacion,buffer+*desplazamiento, transformaciones[i].largoArchivo);
		*desplazamiento+=transformaciones[i].largoArchivo;
		//free(transformaciones[i].ip);
		//free(transformaciones[i].archivoTransformacion);
	}
}


void deserializarReduccionesLocales(t_reduccionLocalMaster* reducciones, int cantRedLocales, void* buffer, int* desplazamiento){
	int i;

	for(i=0;i<cantRedLocales;i++){

		memcpy(&reducciones[i].idNodo,buffer+*desplazamiento,  sizeof(reducciones[i].idNodo));
		*desplazamiento+=sizeof(reducciones[i].idNodo);
		memcpy(&reducciones[i].puerto, buffer+*desplazamiento, sizeof(reducciones[i].puerto));
		*desplazamiento+=sizeof(reducciones[i].puerto);

		memcpy(&reducciones[i].largoIp, buffer+*desplazamiento, sizeof(reducciones[i].largoIp));
		*desplazamiento+=sizeof(reducciones[i].largoIp);
		reducciones[i].ip = malloc(reducciones[i].largoIp);
		memcpy(reducciones[i].ip, buffer+*desplazamiento, reducciones[i].largoIp);
		*desplazamiento+=reducciones[i].largoIp;

		memcpy(&reducciones[i].largoArchivoTransformacion, buffer+*desplazamiento, sizeof(reducciones[i].largoArchivoTransformacion));
		*desplazamiento+=sizeof(reducciones[i].largoArchivoTransformacion);
		reducciones[i].archivoTransformacion = malloc(reducciones[i].largoArchivoTransformacion);
		memcpy(reducciones[i].archivoTransformacion, buffer+*desplazamiento, reducciones[i].largoArchivoTransformacion);
		*desplazamiento+=reducciones[i].largoArchivoTransformacion;

		memcpy(&reducciones[i].largoArchivoRedLocal, buffer+*desplazamiento, sizeof(reducciones[i].largoIp));
		*desplazamiento+=sizeof(reducciones[i].largoArchivoRedLocal);
		reducciones[i].archivoRedLocal = malloc(reducciones[i].largoArchivoRedLocal);
		memcpy(reducciones[i].archivoRedLocal,buffer+*desplazamiento, reducciones[i].largoArchivoRedLocal);
		*desplazamiento+=reducciones[i].largoArchivoRedLocal;
		//free(transformaciones[i].ip);
		//free(transformaciones[i].archivoTransformacion);
	}
}


void deserializarReduccionesGlobales(t_reduccionGlobalMaster* reduccionesGlobales, int cantRedGlobales, void* buffer, int* desplazamiento){
	int i;

	for(i=0;i<cantRedGlobales;i++){

		memcpy(&reduccionesGlobales[i].idNodo,buffer+*desplazamiento,  sizeof(reduccionesGlobales[i].idNodo));
		*desplazamiento+=sizeof(reduccionesGlobales[i].idNodo);
		memcpy(&reduccionesGlobales[i].encargado, buffer+*desplazamiento, sizeof(reduccionesGlobales[i].encargado));
		*desplazamiento+=sizeof(reduccionesGlobales[i].encargado);
		memcpy(&reduccionesGlobales[i].puerto, buffer+*desplazamiento, sizeof(reduccionesGlobales[i].puerto));
		*desplazamiento+=sizeof(reduccionesGlobales[i].puerto);
		memcpy(&reduccionesGlobales[i].largoIp, buffer+*desplazamiento, sizeof(reduccionesGlobales[i].largoIp));
		*desplazamiento+=sizeof(reduccionesGlobales[i].largoIp);
		reduccionesGlobales[i].ip = malloc(reduccionesGlobales[i].largoIp);
		memcpy(reduccionesGlobales[i].ip, buffer+*desplazamiento, reduccionesGlobales[i].largoIp);
		*desplazamiento+=reduccionesGlobales[i].largoIp;

		memcpy(&reduccionesGlobales[i].largoArchivoRedLocal, buffer+*desplazamiento, sizeof(reduccionesGlobales[i].largoIp));
		*desplazamiento+=sizeof(reduccionesGlobales[i].largoIp);
		reduccionesGlobales[i].archivoRedLocal = malloc(reduccionesGlobales[i].largoArchivoRedLocal);
		memcpy(reduccionesGlobales[i].archivoRedLocal,buffer+*desplazamiento, reduccionesGlobales[i].largoArchivoRedLocal);
		*desplazamiento+=reduccionesGlobales[i].largoArchivoRedLocal;

		memcpy(&reduccionesGlobales[i].largoArchivoRedGlobal, buffer+*desplazamiento, sizeof(reduccionesGlobales[i].largoArchivoRedGlobal));
		*desplazamiento+=sizeof(reduccionesGlobales[i].largoArchivoRedGlobal);
		reduccionesGlobales[i].archivoRedGlobal = malloc(reduccionesGlobales[i].largoArchivoRedGlobal);
		memcpy(reduccionesGlobales[i].archivoRedGlobal,buffer+*desplazamiento, reduccionesGlobales[i].largoArchivoRedGlobal);
		*desplazamiento+=reduccionesGlobales[i].largoArchivoRedGlobal;

		//free(transformaciones[i].ip);
		//free(transformaciones[i].archivoTransformacion);
	}
}
