#ifndef FUNCIONESDIRECTORIOS_H_
#define FUNCIONESDIRECTORIOS_H_

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>

typedef struct {
	int index;
	char nombre[255];
	int padre;
} t_directory;

/* Extern Storage Class (clase de almacenamiento externo):
 * se utiliza para dar una referencia de una variable
 * global que es visible para TODOS los archivos de programa.
 * Declaro las variables, que luego seran definidas en otro lado.
 * */
extern t_directory directoriosAGuardar[100];
extern t_directory directoriosGuardados[100];

void stringAppend(char** original, char* stringToAdd);

// Verifica la existencia del directorio metadata en el path dado, si no existe lo crea.
void validarMetadata(char* path);

// Persiste un array de directorios en el archivo path/metadata/directorios.dat (la ruta se genera sola)
void persistirDirectorios(t_directory directorios[], char* path);

// Carga el array pasado como argumento con los directorios que se encuentran almacenados en el archivo path/metadata/directorios.dat.
void obtenerDirectorios(t_directory directorios[], char* path);

void mostrar(t_directory directorios[]);

#endif
