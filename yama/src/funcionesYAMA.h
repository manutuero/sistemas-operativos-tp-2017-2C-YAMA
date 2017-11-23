/*
 * funcionesYAMA.h
 *
 *  Created on: 20/9/2017
 *      Author: utnso
 */

#ifndef FUNCIONESYAMA_H_
#define FUNCIONESYAMA_H_



#include <stdio.h>
#include <stdlib.h>
#include <utils.h>



struct sockaddr_in direccionFS;
struct sockaddr_in direccionYama;
int socketYama, socketFS;
char* ip,*socketFSNodos;
extern uint32_t ultimoMaster;
uint32_t job;
char* FS_IP;
int FS_PUERTO,PUERTO_FS_NODOS;

typedef struct{
	uint32_t idNodo;
	uint32_t puerto;
	uint32_t largoIp;
	char *IP;
}t_infoNodos;

/*  FUNCIONES YAMA */

/* Carga del archivo de configuracion */
void cargarArchivoDeConfiguracion();

/* instancia la conexion al FileSystem */
void conectarseAFS();

/* Arma el select para los masters */
void yamaEscuchando();
void escucharMasters();

/* Arma el Select para las actualizaciones de nodos */
void escuchaActualizacionesNodos();

t_infoNodos deserializarActualizacion(void*);

/* recibe el  nombre del archivo a procesar */
int recibirRutaDeArchivoAProcesar(int,t_rutaArchivo**);

t_rutaArchivo* deserializarRutaArchivo(void* buffer);
void* obtenerBloquesDelArchivo(t_rutaArchivo*);

/* Crea la tabla de estados */
void crearTablaDeEstados();



#endif /* FUNCIONESYAMA_H_ */
