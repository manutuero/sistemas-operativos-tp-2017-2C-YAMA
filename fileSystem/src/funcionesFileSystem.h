#ifndef FUNCIONALMACENARARCHIVOFILESYSTEM_SRC_COSASDELFILESYSTEM_H_
#define FUNCIONALMACENARARCHIVOFILESYSTEM_SRC_COSASDELFILESYSTEM_H_

#include <stdbool.h>
#include "consola/funcionesConsola.h"
#include "API/fileSystemAPI.h"
#include <errno.h>
#include <dirent.h>
#include "test/suite.h"
#include "utils.h"

#define BACKLOG 3

/* Enums */
enum resultadosDeOperacion {
	ERROR = -1, EXITO
};

enum estados {
	ESTABLE, NO_ESTABLE
};

/* Variables globales */
char *ARCHCONFIG;
int PUERTO;
int CANTIDAD_NODOS_ESPERADOS;
char *PATH_METADATA;
int socketNodoConectado;
extern int estadoFs;

/*********************** Estructuras ************************/

typedef struct {
	uint32_t sdNodo;
	uint32_t idNodo;
	uint32_t cantidadBloques;
	uint32_t puerto;
	char *ip;
} t_infoNodo; // luego se eliminara..

/* Estructuras de directorios */
typedef struct {
	int index;
	char nombre[255];
	int padre;
} t_directory;

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

extern t_directory directorios[100];
extern t_list *nodos;

/*********************** Firmas de funciones ************************/

/* Firmas de funciones para archivo de configuracion */
void cargarArchivoDeConfiguracionFS(char *path);

/* Firmas de funciones para inicializacion */
void crearMetadata();
void crearTablaDeDirectorios();
void crearTablaDeArchivos();
void crearTablaDeNodos();
void crearBitmaps();

/* Firmas de funciones para nodos */
void agregarNodo(t_nodo *nodo);

/* Firmas de funciones para bitmaps */
// Crea un array de tipo t_bitmap y lo carga al archivo.
char* persistirBitmap(uint32_t idNodo, int tamanioDatabin);
int verificarExistenciaArchBitmap(char*, char*);
void crearArchivoBitmapNodo(int, int);
void liberarBloqueBitmapNodo(int, int);
void ocuparBloqueBitmapNodo(int, int);
char* obtenerPathBitmap(int);

/* Firmas de funciones para mensajes */
void* esperarConexionesDatanodes();
void* serializarInfoNodo(t_infoNodo *infoNodo, t_header *header);
t_infoNodo deserializarInfoNodo(void *mensaje, int tamanioPayload);

/* Firmas de funciones para directorios */
void crearTablaDeDirectorios();
// Verifica la existencia del directorio en el array de directorios cargado en memoria.
int existeDirectorio(char *path, int *padre);
// Implementacion del comando mkdir de consola.
void mkdirFs(char *path);
// Buscar primer indice vacio del array de directorios.
int buscarPrimerLugarLibre(void);
// Hasta encontrar una mejor forma si cambia el struct t_directory en nombre a char* eliminar
void cargarNombre(char *, int);
// Dada una posicion libre del array de directorio carga en la misma los datos del nuevo directorio
void crearDirectorioLogico(char*, int, int);
// Crea el directorio propiamente dicho(en FS de linux)
void crearDirectorioFisico(int);
// Posible implementacion de ls
void mostrar(t_directory directorios[], int cantidad);
bool existePathDirectorio(char *path);

/* Firmas de funciones para validaciones */
bool hayEstadoAnterior();
void restaurarEstructurasAdministrativas();
void restaurarTablaDeDirectorios();
void restaurarTablaDeNodos();
void validarMetadata(char* path);

/* Auxiliares */
char* getResultado(int);
void stringAppend(char** original, char* stringToAdd);
int traerBloqueNodo(int nodo, uint32_t numBloque, void *bloque);
int guardarBloqueEnNodo(int, uint32_t, void*);
int obtenerYReservarBloqueBitmap(t_bitmap bitmap,int tamanioBitmap);

#endif
