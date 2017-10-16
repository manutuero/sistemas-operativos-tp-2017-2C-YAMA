#ifndef API_FILESYSTEMAPI_H_
#define API_FILESYSTEMAPI_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <stdarg.h>
#include <stdint.h>

#define UN_MEGABYTE 1048576
#define UN_BLOQUE sizeof(char)*UN_MEGABYTE
#define TRUE 1

enum tipoDeArchivo {
	BINARIO, TEXTO
};

typedef struct {
	uint32_t numeroBloque;
	size_t bytesOcupados;
	char *contenido;
} t_bloque;

void escribirStreamConFormato(FILE *stream, char *format, ...);
char* nuevoArchivo();
int almacenarArchivo(char* path, char* nombreArchivo, int tipo, FILE *datos); // stream de datos
void liberarBLoque(t_bloque* bloque);
int proximoRegistro(FILE *datos, char *registro);
int parsearArchivoDeTexto(FILE *datos);
int parsearArchivoBinario(FILE *datos);
void limpiar(char* string, size_t largo);
t_bloque* nuevoBloque(uint32_t numeroBloque);

#endif
