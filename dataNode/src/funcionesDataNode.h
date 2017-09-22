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

char* NODOARCHCONFIG;
char* IP_FILESYSTEM;
int PUERTO_FILESYSTEM;
char* ID_NODO;
int PUERTO_WORKER;
char* RUTA_DATABIN;

void cargarArchivoConfiguracion(char*);

#endif /* FUNCIONESDATANODE_H_ */
