#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/select.h>
#include <commons/config.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>

typedef struct{
	uint32_t id;
	uint32_t tamanio;
} __attribute__((packed)) t_header;

typedef struct {
	uint32_t tamanio;
	char* contenido;
}__attribute__((packed)) t_archivo;

typedef struct{
	uint32_t tamanio;
	char* ruta;
}__attribute__((packed)) t_rutaArchivo;

/* Variables globales */
int PUERTO;

void* deserializar(void*, t_header);
int recibirHeader(int, t_header*);
void* recibirPaquete(int, t_header);
int recibirPorSocket(int, void *, int);
int enviarPorSocket(int, const void*, int);
int nuevoSocket();
int conectarSocket(int , const char*, int);
void enviarPaquete(int, void*, t_header);
void cerrarSocket(int);
int sonIguales(char*, char*); // devuelve la diferencia del largo de la primer cadena con la segunda

/*funciones de envio de archivos*/
void enviarArchivo(int fd, char* buffer, char* archivo);
void serializarYEnviarArchivo(int fd, int tamanio, char* contenido);
void *serializarArchivo(int tamanio, char* contenido, t_header* header);
void* serializarRutaArchivo(t_header* header,t_rutaArchivo* ruta);

#endif
