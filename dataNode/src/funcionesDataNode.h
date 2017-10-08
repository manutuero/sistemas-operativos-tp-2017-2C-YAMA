#ifndef FUNCIONESDATANODE_H_
#define FUNCIONESDATANODE_H_

#include <utils.h>

/* Macros */
#define FAIL 0
#define ERROR 0
#define UN_MEGABYTE 1048576
#define UN_BLOQUE sizeof(char)*UN_MEGABYTE

/* Variables globales */
char* NODOARCHCONFIG;
char* IP_FILESYSTEM;
int PUERTO_FILESYSTEM;
char* ID_NODO;
int PUERTO_WORKER;
char* RUTA_DATABIN;

/* TADS */
typedef struct {
	uint32_t sdNodo;
	uint32_t idNodo;
	uint32_t cantidadBloques;
	uint32_t puerto;
	char *ip;
} t_infoNodo;

/* Firmas de funciones */
void setBloque(int numero, char* datos);
char* getBloque(int numero);
void* serializarInfoNodo(t_infoNodo*, t_header*);
void cargarArchivoConfiguracionDatanode(char*);
int conectarAfilesystem(char*, int);

#endif
