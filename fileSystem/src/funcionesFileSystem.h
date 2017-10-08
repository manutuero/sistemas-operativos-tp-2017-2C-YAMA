#ifndef FUNCIONALMACENARARCHIVOFILESYSTEM_SRC_COSASDELFILESYSTEM_H_
#define FUNCIONALMACENARARCHIVOFILESYSTEM_SRC_COSASDELFILESYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <commons/config.h>
#include <commons/string.h>
#include "consola/funcionesConsola.h"
#include "utils/utils.h"

#define BACKLOG 3

/* Enums */
enum tiposDeArchivos {BINARIO, TEXTO};
enum resultadosOperacion {ERROR = -1, EXITO};

/* Variables globales */
char *ARCHCONFIG;
int PUERTO;
extern int estadoFs;

/* 								Estructuras										*/

/* Estructuras de bitmaps */
typedef struct {
	char estadoBLoque;
}t_bitMap;

/* Estructuras de nodos */
typedef struct {
	uint32_t sdNodo;
	uint32_t idNodo;
	uint32_t cantidadBloques;
	uint32_t puerto;
	char *ip;
} t_infoNodo;

/* Estructuras de directorios */

typedef struct {
	int index;
	char nombre[255];
	int padre;
} t_directory;

extern t_directory directorios[100];

/* Estructuras de archivos */

typedef struct bloque{
	int tamanioBloque;
	int nodoCopia0;
	int bloqueCopia0;
	int nodoCopia1;
	int bloqueCopia1;
	struct bloque* siguiente;
}t_bloque;

typedef struct{
	char* nombre;
	char tipo;
	int tamanio;
	t_bloque primerBloque;
}composicionArchivo;

/* 							Firmas de funciones 							*/

/* Firma funciones de archivo de configuracion */
void cargarArchivoDeConfiguracion(char*);

/* Firma funciones de bitmaps */

/* Este path es el que yo use,se tiene que definir donde dejar la carpeta metadata
Para correr estas funciones cada uno deberia modificar el path para que le funcione */
char* pathBitmap = "/home/utnso/Desarrollo/tp-2017-2c-The_Ponchos/fileSystem/metadata/bitmaps/";
void cargarArchivoBitmap(FILE*,int);
int verificarExistenciaArchBitmap(char*,char*);
void crearArchivoBitmapNodo(int ,int);
void liberarBloqueBitmapNodo(int,int);
void ocuparBloqueBitmapNodo(int,int);
char* armarNombreArchBitmap(int );


/* Firma funciones de Sockets */

int nuevoSocket();

int recibirPorSocket(int, void*, int);

t_infoNodo deserializarInfoNodo(void*, int);

void* esperarConexionesDatanodes();

/*	La funcion recibirá una ruta completa, el nombre del archivo, el tipo (texto o binario) y los datos
	correspondientes. Responderá con un mensaje confirmando el resultado de la operación.*/
int almacenarArchivo(char*, char*, int, char*); // Datos podria ser un struct en vez de un char* ... cosa para discutir en grupo.

char* getResultado(int);

void* serializarInfoNodo(t_infoNodo*, t_header*);

/* Firma Funciones de directorios */

//Verifica la existencia del directorio en el array de directorios en memoria
int existeDirectorio(char*,int*);

//Implentacion del mkdir de consola
void mkDirFS(char *);

//Buscar primer indice vacio del array de directorios
int buscarPrimerLugarLibre(void);

//Hasta encontrar una mejor forma si cambia el struct t_directory en nombre a char* eliminar
void cargarNombre(char *, int);

//Dada una posicion libre del array de directorio carga en la misma los datos
//del nuevo directorio
void crearDirectorioLogico(char*,int,int);

//Crea el directorio propiamente dicho(en FS de linux)
void crearDirectorioFisico(int);

//Funcion que reemplaza la de las commons
void stringAppend(char** original, char* stringToAdd);

// Verifica la existencia del directorio metadata en el path dado, si no existe lo crea.
void validarMetadata(char* path);

// Persiste un array de directorios en el archivo path/metadata/directorios.dat (la ruta se genera sola)
void persistirDirectorios(t_directory directorios[], char* path);

// Carga el array pasado como argumento con los directorios que se encuentran almacenados en el archivo path/metadata/directorios.dat.
void obtenerDirectorios(t_directory directorios[], char* path);

// Posible implementacion de LS
void mostrar(t_directory directorios[]);

#endif
