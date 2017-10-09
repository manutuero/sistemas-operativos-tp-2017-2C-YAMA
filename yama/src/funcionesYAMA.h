#ifndef FUNCIONESYAMA_H_
#define FUNCIONESYAMA_H_

#include <utils.h>

/* Variables globales */
struct sockaddr_in direccionFS;
struct sockaddr_in direccionYama;
int socketYama;

typedef struct{
	uint32_t disponible;
	uint32_t nroBloqueArch;
	uint32_t idNodo;
	uint32_t nroBloqueNodo;
}t_bloqueRecv;

typedef struct{
	uint32_t idNodo;
	uint32_t nroBloqueNodo;
	uint32_t puerto;
	char* ip;
}t_infoMaster;

typedef struct{
	uint32_t idNodo;
	uint32_t puerto;
	char* ip;
}t_worker_Disponibles;

t_worker_Disponibles workers[30];



/* Firmas de funciones*/
void cargarArchivoDeConfiguracion();
void conectarseAFS();
void yamaEscuchando();
void recibirRutaDeArchivoAProcesar(int);
t_rutaArchivo* deserializarRutaArchivo(void* buffer);

#endif
