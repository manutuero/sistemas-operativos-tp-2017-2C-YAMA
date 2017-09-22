/*
 * funcionesWorker.h
 *
 *  Created on: 13/9/2017
 *      Author: utnso
 */

#ifndef FUNCIONESWORKER_H_
#define FUNCIONESWORKER_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <string.h>

t_log* workerLogger;
char* NODOARCHCONFIG;
char* IP_FILESYSTEM;
int PUERTO_FILESYSTEM;
char* ID_NODO;
int PUERTO_WORKER;
char* RUTA_DATABIN;

typedef struct header{
	int id;
	int tamanio;
}__attribute__((packed)) myHeader;

typedef struct archivo{
	uint32_t tamanio;
	char* contenido;
}__attribute__((packed)) archivo;

void crearLogger();
void cargarArchivoConfiguracion(char*);
int recibirArchivo(int cliente);
int recibirYDeserializar(int fd, archivo* miArchivo);
archivo *deserializarArchivo(void *buffer, int tamanio);


#endif /* FUNCIONESWORKER_H_ */
