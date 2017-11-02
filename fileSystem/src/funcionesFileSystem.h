#ifndef FUNCIONALMACENARARCHIVOFILESYSTEM_SRC_COSASDELFILESYSTEM_H_
#define FUNCIONALMACENARARCHIVOFILESYSTEM_SRC_COSASDELFILESYSTEM_H_

#include <stdbool.h>
#include "consola/funcionesConsola.h"
#include "API/fileSystemAPI.h"
#include <errno.h>
#include <dirent.h>
#include "test/suite.h"
#include "utils.h"
#include <semaphore.h> // Para probar...
#define BACKLOG 3
#define CANTIDAD_DIRECTORIOS 100

/* Enums */
enum resultadosDeOperacion {
	ERROR = -1, EXITO
};

enum estados {
	ESTABLE, NO_ESTABLE
};

enum directorios {
	DIR_NO_EXISTE = 100
};

enum bitmaps {
	ESTA_LLENO = -1
};

enum nodos {
	NO_DISPONIBLE = -1
};

enum respuestasDatanode {
	ERROR_AL_ENVIAR_PETICION = -3,
	ERROR_AL_TRAER_BLOQUE,
	ERROR_AL_RECIBIR_RESPUESTA,
	ERROR_NO_SE_PUDO_GUARDAR_BLOQUE,
	GUARDO_BLOQUE_OK,
	TRAJO_BLOQUE_OK
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
typedef char* t_directorio;

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
extern t_list *archivos;


extern sem_t sem;
/*********************** Firmas de funciones ************************/

/* Firmas de funciones para archivo de configuracion */
void cargarArchivoDeConfiguracionFS(char *path);

/* Firmas de funciones para inicializacion */
void crearDirectorioMetadata();
void crearTablaDeDirectorios();
void crearDirectorioArchivos();
void crearTablaDeNodos();
void crearDirectorioBitmaps();

/* Firmas de funciones para nodos */
void agregarNodo(t_list *lista, t_nodo *nodo);
void persistirTablaDeNodos();
void actualizarTablaDeNodos();
int totalBloquesFileSystem();
int bloquesLibresFileSystem();
int obtenerSocketNodo(t_bloque *bloque);

/* Firmas de funciones para bitmaps */
// Crea un array de tipo t_bitmap y lo carga al archivo.
char* persistirBitmap(uint32_t idNodo, int tamanioDatabin);
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
void mostrar(int cantidad);
bool existePathDirectorio(char *path);
int obtenerIndice(char *directorio);
t_directorio obtenerPathDirectorio(char *path);
void moverDirectorio(t_directorio pathOriginal, t_directorio pathFinal);

/* Firmas de funciones para tabla de archivos */
void persistirTablaDeArchivos(char *path);
t_archivo_a_persistir* obtenerArchivo(char *path);
t_archivo_a_persistir* buscarArchivoPorIndiceYNombre(int indice,
		char *nombreArchivo);
char* obtenerNombreArchivo(char *path);
void renombrarArchivo(char *pathOriginal, char *nombreFinal);
void renombrarDirectorio(char *pathOriginal, char *nombreFinal);
void moverArchivo(char *pathOriginal, t_directorio pathFinal);
t_archivo_a_persistir* abrirArchivo(char *path);

/* Firmas de funciones para validaciones */
bool hayEstadoAnterior();
void validarMetadata(char* path);

/* Firmas de funciones para restaurar un estado anterior */
void restaurarEstructurasAdministrativas();
void restaurarTablaDeDirectorios();
void restaurarTablaDeNodos();
void restaurarTablaDeArchivos();

/* Auxiliares */
char* getResultado(int);
void stringAppend(char** original, char* stringToAdd);
int traerBloqueNodo(t_bloque *bloque);
int guardarBloqueEnNodo(uint32_t numeroBloque, void *bloque, int socketNodo);
int obtenerYReservarBloqueBitmap(t_bitmap bitmap, int tamanioBitmap);
t_list* copiarListaNodos(t_list *lista);
int lastIndexOf(char *cadena, char caracter);

#endif
