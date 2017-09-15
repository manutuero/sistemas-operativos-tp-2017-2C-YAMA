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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdint.h>
#include <netdb.h>


typedef struct header{
	int id;
	int tamanio;
}__attribute__((packed))myHeader;

typedef struct archivo{
	uint32_t tamanio;
	char* contenido;
}__attribute__((packed)) archivo;



long calcularTamanioArchivo(FILE* fich, char* archivo);
void enviarArchivo(int fd, FILE* fich, char* buffer, long * tam, char* archivo);
void serializarYEnviarArchivo(int fd, int tamanio, char* contenido);
void *serializarArchivo(int tamanio, char* contenido, myHeader* header);
int chequearParametros(char *transformador,char *reductor,char *archivoAprocesar,char *direccionDeResultado);
int file_exists (char * fileName);

#endif /* FUNCIONESMASTER_H_ */
