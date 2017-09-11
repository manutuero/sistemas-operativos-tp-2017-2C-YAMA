/*
 * funcionesMaster.h
 *
 *  Created on: 10/9/2017
 *      Author: utnso
 */

#ifndef FUNCIONESMASTER_H_
#define FUNCIONESMASTER_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>

int chequearParametros(char *transformador,char *reductor,char *archivoAprocesar,char *direccionDeResultado);


#endif /* FUNCIONESMASTER_H_ */
