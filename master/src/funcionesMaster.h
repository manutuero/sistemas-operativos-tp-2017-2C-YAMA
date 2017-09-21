/*
 * funcionesMaster.h
 *
 *  Created on: 11/9/2017
 *      Author: utnso
 */


#ifndef FUNCIONESMASTER_H_
#define FUNCIONESMASTER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/mman.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdint.h>
#include <netdb.h>
#include <fcntl.h>
#include <commons/string.h>
#include "utils.h"



typedef struct header{
	int id;
	int tamanio;
}__attribute__((packed))myHeader;

typedef struct archivo{
	uint32_t tamanio;
	char* contenido;
}__attribute__((packed)) archivo;


void enviarArchivo(int fd, char* buffer, char* archivo);
void serializarYEnviarArchivo(int fd, int tamanio, char* contenido);
void *serializarArchivo(int tamanio, char* contenido, myHeader* header);
int chequearParametros(char *transformador,char *reductor,char *archivoAprocesar,char *direccionDeResultado);
int file_exists (char * fileName);
void iniciarMaster(char* transformador,char* reductor,char* archivoAprocesar,char* direccionDeResultado);
void conectarseAYama(int puerto,char* ip);

#endif /* FUNCIONESMASTER_H_ */
