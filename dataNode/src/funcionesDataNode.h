/*
 * funcionesDataNode.h
 *
 *  Created on: 21/9/2017
 *      Author: utnso
 */

#ifndef FUNCIONESDATANODE_H_
#define FUNCIONESDATANODE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/select.h>
#include <commons/string.h>
#include <netinet/in.h>
#include <arpa/inet.h> // sin esto explota en mil pedazos es para parsear la direccion de internet !!

#define FAIL 0
#define ERROR 0

char* NODOARCHCONFIG;
char* IP_FILESYSTEM;
int   PUERTO_FILESYSTEM;
char* ID_NODO;
int   PUERTO_WORKER;
char* RUTA_DATABIN;

typedef struct {
    uint32_t sdNodo;
    uint32_t idNodo;
    uint32_t cantidadBloques;
    uint32_t puerto;
    char *ip;
} t_infoNodo;

typedef struct{
	uint32_t id;
	uint32_t tamanio;
} __attribute__((packed)) t_header;


void* serializarInfoNodo(t_infoNodo*, t_header*);
void cargarArchivoConfiguracion(char*);
int conectarAfilesystem(char*, int);
void cerrarSocket(int unSocket);

#endif
