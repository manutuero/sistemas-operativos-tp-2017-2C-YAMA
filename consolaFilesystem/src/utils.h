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

enum headers {SOLICITUD_EJECUTAR_COMANDO_CONSOLA = 1};

typedef struct{
	int funcion;
	int opcion;
	char *parametro1;
	char *parametro2;
	int bloque;
	int idNodo;
} comando;

typedef struct{
	int id;
	int tamanio;
} header;

/* Recibe un mensaje serializado y devuelve un puntero generico (void) al buffer de memoria donde estara la respuesta deserializada del mensaje. */
void* deserializar(void*, header);
void* deserializarSolicitudEjecutarComando(void*);
int recibirHeader(int, header*);
void* recibirPaquete(int, header);
int recibirPorSocket(int, void *, int);
int enviarPorSocket(int, const void*, int);
int nuevoSocket();
int conectarSocket(int , const char*, int );
void enviarPaquete(int, void*, header );
void cerrarSocket(int);
#endif /* UTILS_H_ */
