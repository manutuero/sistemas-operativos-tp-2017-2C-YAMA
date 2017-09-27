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
//#include "../../fileSystem/src/utils/utils.h"


/*
typedef struct rutaArchivo{
	int tamanio;
	char* ruta;
}__attribute__((packed)) t_rutaArchivo;
*/

int chequearParametros(char *transformador,char *reductor,char *archivoAprocesar,char *direccionDeResultado);
int file_exists (char * fileName);
void iniciarMaster(char* transformador,char* reductor,char* archivoAprocesar,char* direccionDeResultado);
int conectarseAYama(int puerto,char* ip);
void mandarRutaArchivoAYama(int socketYama, char* archivoAprocesar);

#endif /* FUNCIONESMASTER_H_ */
