#ifndef API_FILESYSTEMAPI_H_
#define API_FILESYSTEMAPI_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <stdarg.h>
#include <stdint.h>
#include <semaphore.h>

#define UN_MEGABYTE 1048576
#define UN_BLOQUE sizeof(char)*UN_MEGABYTE
#define TRUE 1

enum tipoDeArchivo {
	BINARIO, TEXTO
};

/* Estructuras de bitmaps */
typedef char* t_bitmap;

/* Estructuras de nodos */
typedef struct {
	uint32_t socketDescriptor;
	uint32_t idNodo;
	uint32_t bloquesTotales;
	uint32_t bloquesLibres;
	uint32_t puertoWorker;
	t_bitmap bitmap;
	char *ip;
} t_nodo;

/* Estructuras de archivo */
typedef struct {
	uint32_t numeroBloque;
	size_t bytesOcupados;
	char *contenido;
	char disponible;
	int numeroBloqueCopia0;
	int numeroBloqueCopia1;
	t_nodo *nodoCopia0;
	t_nodo *nodoCopia1;
} t_bloque;

typedef struct {
	int indiceDirectorio;
	char *nombreArchivo;
	int tamanio;
	int tipo;
	char disponible;
	t_list *bloques;
} t_archivo_a_persistir; // nombre temporal...cambiar a t_archivo cuando hagan refactor de la utils.

typedef struct {
	t_bloque *bloque;
	int copia;
} t_arg;

/* Semaforos */
extern pthread_mutex_t mutex;
extern sem_t semCopia0, semCopia1;

/* API */
int almacenarArchivo(char *pathDirectorio, char *nombreArchivo, int tipo,
		FILE *datos); // stream de datos

t_archivo_a_persistir* leerArchivo(char *pathArchivo);

/* Metadata */
void actualizarBitmaps();

/* Auxiliares*/
void escribirStreamConFormato(FILE *stream, char *format, ...);
int proximoRegistro(FILE *datos, char *registro);
t_list* obtenerBloques(FILE *datos, int tipo);
t_list* parsearArchivoDeTexto(FILE *datos);
t_list* parsearArchivoBinario(FILE *datos);
void limpiar(char* string, size_t largo);
t_bloque* nuevoBloque(uint32_t numeroBloque);
void ordenarListaNodos(t_list *nodos);
bool compararBloquesLibres(t_nodo *unNodo, t_nodo *otroNodo);
bool compararPorIdDesc(t_nodo *unNodo, t_nodo *otroNodo);
void destruirNodo(t_nodo *nodo);
t_archivo_a_persistir* nuevoArchivo(char *path, char *nombreArchivo, int tipo,
		int tamanio, t_list *bloques);
void crearTablaDeArchivo(t_archivo_a_persistir *archivo);
int validarGuardado(int respuesta, t_bloque *bloque, t_nodo *nodo);

// Liberar memoria.
void liberarNodo(t_nodo *nodo);

void liberarArchivo(t_archivo_a_persistir *archivo);
void liberarArchivoYNodos(t_archivo_a_persistir *archivo);
void liberarArchivoSinContenido(t_archivo_a_persistir *archivo);

void liberarBloque(t_bloque *bloque);
void liberarBloqueYNodos(t_bloque *bloque);
void liberarBloqueSinContenido(t_bloque *bloque);
void liberarBloqueSinContenidoYNodos(t_bloque *bloque);

void liberarBloques(t_list *bloques);
void liberarBloquesYNodos(t_list *bloques);

#endif
