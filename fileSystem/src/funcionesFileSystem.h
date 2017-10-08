#ifndef FUNCIONALMACENARARCHIVOFILESYSTEM_SRC_COSASDELFILESYSTEM_H_
#define FUNCIONALMACENARARCHIVOFILESYSTEM_SRC_COSASDELFILESYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "consola/funcionesConsola.h"
#include "utils.h"

#define BACKLOG 3

/* Enums */
enum tiposDeArchivos {BINARIO, TEXTO};
enum resultadosOperacion {ERROR = -1, EXITO};

/* Variables globales */
char *ARCHCONFIG;
int PUERTO;
extern int estadoFs;

typedef struct {
	char estadoBLoque;
}t_bitMap;

typedef struct {
	uint32_t sdNodo;
	uint32_t idNodo;
	uint32_t cantidadBloques;
	uint32_t puerto;
	char *ip;
} t_infoNodo;

/* Firmas de funciones */
void cargarArchivoDeConfiguracion(char*);

/* Este path es el que yo use,se tiene que definir donde dejar la carpeta metadata
Para correr estas funciones cada uno deberia modificar el path para que le funcione */
char* pathBitmap = "/home/utnso/Desarrollo/tp-2017-2c-The_Ponchos/fileSystem/metadata/bitmaps/";
void cargarArchivoBitmap(FILE*,int);
int verificarExistenciaArchBitmap(char*,char*);
void crearArchivoBitmapNodo(int ,int);
void liberarBloqueBitmapNodo(int,int);
void ocuparBloqueBitmapNodo(int,int);
char* armarNombreArchBitmap(int );

int nuevoSocket();

int recibirPorSocket(int, void*, int);

t_infoNodo deserializarInfoNodo(void*, int);

void* esperarConexionesDatanodes();

/*	La funcion recibirá una ruta completa, el nombre del archivo, el tipo (texto o binario) y los datos
	correspondientes. Responderá con un mensaje confirmando el resultado de la operación.*/
int almacenarArchivo(char*, char*, int, char*); // Datos podria ser un struct en vez de un char* ... cosa para discutir en grupo.

char* getResultado(int);

void* serializarInfoNodo(t_infoNodo*, t_header*);

#endif
