#ifndef FUNCIONALMACENARARCHIVOFILESYSTEM_SRC_COSASDELFILESYSTEM_H_
#define FUNCIONALMACENARARCHIVOFILESYSTEM_SRC_COSASDELFILESYSTEM_H_

#include "consola/funcionesConsola.h"
#include "API/fileSystemAPI.h"
#include <errno.h>
#include <dirent.h>

#define BACKLOG 3

/* Enums */
enum resultadosDeOperacion {
	ERROR = -1, EXITO
};

/* Variables globales */
char *ARCHCONFIG;
int PUERTO;
extern int estadoFs;
/* Este path es el que yo use,se tiene que definir donde dejar la carpeta metadata
 Para correr estas funciones cada uno deberia modificar el path para que le funcione */
extern char *pathBitmap;

/*********************** Estructuras ************************/

/* Estructuras de bitmaps */
typedef struct {
	char estadoBLoque;
} t_bitMap;

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

int socketNodoConectado;
extern t_directory directorios[100];

/* Estructuras de archivos */
typedef struct bloque {
	int tamanioBloque;
	int nodoCopia0;
	int bloqueCopia0;
	int nodoCopia1;
	int bloqueCopia1;
	struct bloque* siguiente;
} t_bloque_arch;

typedef struct {
	char* nombre;
	char tipo;
	int tamanio;
	t_bloque_arch primerBloque;
} composicionArchivo;

/*********************** Firmas de funciones ************************/
/* Firmas de funciones para archivo de configuracion */
void cargarArchivoDeConfiguracionFS(char *path);

/* Firmas de funciones para bitmaps */
// Crea un array de tipo t_bitmap y lo carga al archivo.
void cargarArchivoBitmap(FILE *archivo, int tamanioDatabin);
int verificarExistenciaArchBitmap(char*, char*);
void crearArchivoBitmapNodo(int, int);
void liberarBloqueBitmapNodo(int, int);
void ocuparBloqueBitmapNodo(int, int);
char* armarNombreArchBitmap(int);

/* Firmas de funciones para mensajes */
void* esperarConexionesDatanodes();
void* serializarInfoNodo(t_infoNodo *infoNodo, t_header *header);
t_infoNodo deserializarInfoNodo(void *mensaje, int tamanioPayload);

/* Firmas de funciones para directorios */
// Verifica la existencia del directorio en el array de directorios cargado en memoria.
int existeDirectorio(char *path, int *padre);
// Implementacion del comando mkdir de consola.
void mkDirFS(char *path);
// Buscar primer indice vacio del array de directorios.
int buscarPrimerLugarLibre(void);
// Hasta encontrar una mejor forma si cambia el struct t_directory en nombre a char* eliminar
void cargarNombre(char *, int);
// Dada una posicion libre del array de directorio carga en la misma los datos del nuevo directorio
void crearDirectorioLogico(char*, int, int);
// Crea el directorio propiamente dicho(en FS de linux)
void crearDirectorioFisico(int);
// Verifica la existencia del directorio metadata en el path dado, si no existe lo crea.
void validarMetadata(char* path);
// Persiste un array de directorios en el archivo path/metadata/directorios.dat (la ruta se genera sola)
void persistirDirectorios(t_directory directorios[], char* path);
// Carga el array pasado como argumento con los directorios que se encuentran almacenados en el archivo path/metadata/directorios.dat.
void obtenerDirectorios(t_directory directorios[], char* path);
// Posible implementacion de ls
void mostrar(t_directory directorios[]);

/* Auxiliares */
char* getResultado(int);
void stringAppend(char** original, char* stringToAdd);
int traerBloqueNodo(int nodo, uint32_t numBloque, void *bloque);
int guardarBloqueEnNodo(int, uint32_t, void*);

#endif
