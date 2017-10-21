/*
 * funcionesMaster.h
 *
 *  Created on: 11/9/2017
 *      Author: utnso
 */


#ifndef FUNCIONESMASTER_H_
#define FUNCIONESMASTER_H_

#include <utils.h>


/*
typedef struct rutaArchivo{
	int tamanio;
	char* ruta;
}__attribute__((packed)) t_rutaArchivo;
*/

/* Utilizada para enviar la etapa de transformacion */
	typedef struct{
		uint32_t idNodo;
		uint32_t nroBloqueNodo;
		uint32_t puerto;
		uint32_t bytesOcupados;
		uint32_t largoIp;
		char* ip;
		uint32_t largoArchivo;
		char* archivoTransformacion;
	}t_transformacionMaster;

/* Utilizada para enviar la etapa de reduccion local */
	typedef struct{
		uint32_t idNodo;
		uint32_t puerto;
		uint32_t largoIp;
		char* ip;
		uint32_t largoArchivoTransformacion;
		char* archivoTransformacion;
		uint32_t largoArchivoRedLocal;
		char* archivoRedLocal;
	}t_reduccionLocalMaster;

/* Utilizada para enviar la etapa de reduccion global*/
	typedef struct{
		uint32_t idNodo;
		uint32_t puerto;
		uint32_t encargado;
		uint32_t largoIp;
		char* ip;
		uint32_t largoArchivoRedLocal;
		char* archivoRedLocal;
		uint32_t largoArchivoRedGlobal;
		char* archivoRedGlobal;
	}t_reduccionGlobalMaster;



int chequearParametros(char *transformador,char *reductor,char *archivoAprocesar,char *direccionDeResultado);
int file_exists (char * fileName);
void iniciarMaster(char* transformador,char* reductor,char* archivoAprocesar,char* direccionDeResultado);
int conectarseAYama(int puerto,char* ip);
void mandarRutaArchivoAYama(int socketYama, char* archivoAprocesar);

void deserializarTransformaciones(t_transformacionMaster* , int , void*, int*);
void deserializarReduccionesLocales(t_reduccionLocalMaster* , int , void* , int* );
void deserializarReduccionesGlobales(t_reduccionGlobalMaster*, int , void* , int*);



#endif /* FUNCIONESMASTER_H_ */
