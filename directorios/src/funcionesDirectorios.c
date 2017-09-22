#include "funcionesDirectorios.h"

/* Hardcodeado por ahora */
t_directory directoriosAGuardar[100] = {
			{ 0, "/", -1 },
			{ 1, "user", 0 },
			{ 2, "jose", 1 },
			{ 3, "juan", 1 },
			{ 4, "temporal", 0 },
			{ 5, "datos", 2 },
			{ 6, "fotos", 2 }
};

t_directory directoriosGuardados[100] = {}; // Inicializo el array de estructuras

void validarMetadata(char* path) {
	char *newPath = malloc(strlen(path) + strlen("/metadata"));

	if (newPath) {
		newPath[0] = '\0';
		strcat(newPath, path);
		strcat(newPath, "/metadata");
	} else {
		fprintf(stderr, "malloc fallido!.\n");
	}

	DIR* directoryPointer = opendir(newPath);

	if (directoryPointer) {
		// El directorio existe.
		closedir(directoryPointer);
	}
	// Si el directorio no existe
	else if (ENOENT == errno) {
		mkdir("metadata", 0777); // Le damos todos los permisos, por ahora.
		closedir(directoryPointer);
	}

	free(newPath);
}

void persistirDirectorios(t_directory directorios[], char* path) {
	int i;
	char *newPath = malloc(strlen(path) + strlen("/metadata/directorios.dat"));

	if (newPath != NULL) {
		newPath[0] = '\0';
		strcat(newPath, path);
		strcat(newPath, "/metadata/directorios.dat");
	} else {
		fprintf(stderr, "malloc fallido!.\n");
	}

	FILE *filePointer = fopen(newPath, "w+b");
	fseek(filePointer, 0, SEEK_SET);

	if (!filePointer) {
		fprintf(stderr, "Error. No se puede abrir el archivo");
	}

	for (i = 0; i < 100; i++) {
		fwrite(&directorios[i], sizeof(t_directory), 1, filePointer);
	}


	fclose(filePointer);
	free(newPath);
}

void obtenerDirectorios(t_directory directorios[], char* path) {
	int i;
	char *newPath = malloc(strlen(path) + strlen("/metadata/directorios.dat"));

	if (newPath) {
		newPath[0] = '\0';
		strcat(newPath, path);
		strcat(newPath, "/metadata/directorios.dat");
	} else {
		fprintf(stderr, "malloc fallido!.\n");
	}

	FILE *filePointer = fopen(newPath, "r+b");
	fseek(filePointer, 0, SEEK_SET);

	if (!filePointer) {
		fprintf(stderr, "Error. No se puede abrir el archivo");
	}

	for (i = 0; i < 100; i++) {
		fread(&directorios[i], sizeof(t_directory), 1, filePointer);
	}

	fclose(filePointer);
}

void mostrar(t_directory directorios[]) {
	int i;
	printf("index			nombre						padre\n");
	for (i = 0; i < 100; i++) {
		printf("%d			%s						%d\n",
				directorios[i].index,
				directorios[i].nombre,
				directorios[i].padre);
	}
}
