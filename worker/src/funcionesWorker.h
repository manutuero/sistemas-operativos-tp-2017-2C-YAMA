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
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>


typedef struct header{
	int id;
	int tamanio;
}__attribute__((packed)) myHeader;

typedef struct archivo{
	uint32_t tamanio;
	char* contenido;
}__attribute__((packed)) archivo;


int recibirArchivo(int bytesRecibidos, int cliente, int len, FILE* fich);
int recibirYDeserializar(int fd, archivo* miArchivo);
archivo *deserializarArchivo(void *buffer, int tamanio);


#endif /* FUNCIONESWORKER_H_ */
