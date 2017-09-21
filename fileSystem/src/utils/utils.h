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

typedef struct{
	int id;
	int tamanio;
} __attribute__((packed)) t_header;

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

#endif
