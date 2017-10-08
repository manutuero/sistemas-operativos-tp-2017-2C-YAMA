#ifndef UTILS_H_
#define UTILS_H_

// Ordenado alfabeticamente de la 'a' a la 'z'
#include <arpa/inet.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/types.h>
#include <unistd.h>

#define FAIL 0

typedef struct{
	uint32_t id;
	uint32_t tamanioPayload;
} __attribute__((packed)) t_header;

typedef struct {
	uint32_t tamanio;
	char* contenido;
}__attribute__((packed)) t_archivo;

typedef struct {
	int funcion;
	int opcion;
	char *parametro1;
	char *parametro2;
	int bloque;
	int idNodo;
} t_comando;

typedef struct{
	uint32_t tamanio;
	char* ruta;
}__attribute__((packed)) t_rutaArchivo;

/* Variables globales */
int PUERTO;

/* Funciones para envio/recepcion de mensajes por sockets */
int nuevoSocket();
int conectarSocket(int , const char*, int);
int enviarPorSocket(int, const void*, int);
int recibirPorSocket(int, void *, int);
int recibirHeader(int, t_header*);
void* recibirPayload(int, t_header);
void enviarPaquete(int, void*, t_header);
void* deserializar(void*, t_header);
void cerrarSocket(int);

/* Funciones de envio de archivos */
void enviarArchivo(int fd, char* buffer, char* archivo);
void serializarYEnviarArchivo(int fd, int tamanio, char* contenido);
void *serializarArchivo(int tamanio, char* contenido, t_header* header);
void* serializarRutaArchivo(t_header* header, t_rutaArchivo* ruta);

/* Otras */
int sonIguales(char*, char*);

#endif
