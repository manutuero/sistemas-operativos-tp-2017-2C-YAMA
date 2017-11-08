#include "funcionesConsola.h"

bool esValido(char *path) {
	return string_starts_with(path, "/root") ? true : false;
}

int esNumero(char* valor) {
	int i = 0;
	while (valor[i] != '\0') {
		if (!isdigit(valor[i])) {
			return 0;
		} else
			i++;
	}
	return 1;
}

int cantidadArgumentos(char** argumentos) {
	int cantidad = 0, i = 0;
	char *palabra;

	palabra = argumentos[i];

	while (palabra != NULL) {
		cantidad++;
		i++;
		palabra = argumentos[i];
	}

	return cantidad;
}

char** cargarArgumentos(char* linea) {
	string_trim(&linea);
	return (string_split(linea, ESPACIO));
}

void ejecutarFormat(char **argumentos) {
	persistirTablaDeNodos();
	// Hacer format..
}

char* invocarFuncionRm(char **argumentos) {
	if (esValido(argumentos[1])) {
		printf("Funcion de remove archivo estandar.\n");
		printf("El archivo a remover es: %s.\n", argumentos[1]);
	} else {
		printf("El parametro ingresado '%s' no es valido.\n", argumentos[1]);
	}

	return "<default>";
}

char* invocarFuncionRmDirectory(char **argumentos) {
	//revisar que directorio esta vacio primero

	if ((strcmp(argumentos[1], "-d") == 0) && (esValido(argumentos[2]))) {

		printf("Funcion de remove directorio.\n");
		printf("El directorio a remover es: %s.\n", argumentos[2]);
	} else
		printf(
				"La opcion: %s no es valida o el parametro: %s no es correcto.\n",
				argumentos[1], argumentos[2]);

	return "<default>";
}

char* invocarFuncionRmBloque(char **argumentos) {
	//revisar que no es la ultima copia del bloque
	if ((strcmp(argumentos[1], "-b") == 0) && (esNumero(argumentos[3]))
			&& (esNumero(argumentos[4]))) {

		printf("Funcion de remover un bloque.\n");
		printf("Se removera del archivo: %s.\n", argumentos[2]);
		printf("El bloque: %s de la copia: %s.\n", argumentos[3],
				argumentos[4]);
	} else
		printf(
				"La opcion: %s no es valida o uno de los parametros: %s,%s no es correcto.\n",
				argumentos[1], argumentos[2], argumentos[3]);

	return "<default>";
}

void ejecutarCat(char **argumentos) {
	int i, indice;
	char *pathArchivo, *contenido;
	t_directorio directorio;
	t_list *bloques;
	off_t offset = 0;
	t_bloque *bloque;
	t_archivo_a_persistir *archivo;

	// Validaciones.
	pathArchivo = argumentos[1];
	if (!esValido(pathArchivo)) {
		printf("La ruta '%s' no es valida.\n", pathArchivo);
		return;
	}

	directorio = obtenerPathDirectorio(pathArchivo);
	indice = obtenerIndice(directorio);
	if (!existePathDirectorio(directorio) || indice == DIR_NO_EXISTE) {
		printf("El directorio '%s' no existe.\n", directorio);
		return;
	}

	// Me traigo el archivo a memoria y muestro su contenido por STDOUT.
	archivo = leerArchivo(pathArchivo);
	bloques = archivo->bloques;
	contenido = malloc(sizeof(char) * archivo->tamanio);
	for (i = 0; i < bloques->elements_count; i++) {
		bloque = list_get(bloques, i);
		memcpy(contenido + offset, bloque->contenido, bloque->bytesOcupados);
		offset += bloque->bytesOcupados;
	}

	// Imprimo el contenido del archivo como texto plano (cat).
	printf("%s", contenido);

	// Libero recursos.
	free(pathArchivo);
	free(contenido);
}

void ejecutarMkdir(char **argumentos) {
	char *path = argumentos[1];
	if (esValido(path) && strlen(path) > 1) {
		mkdirFs(path);
	} else
		printf(
				"mkdir: no se puede crear el directorio «%s»: La ruta ingresada no es valida.\n",
				path);
}

void ejecutarMd5(char **argumentos) {
	char *pathArchivoYamaFs, *nombreArchivo, *pathArchivoTemporal, *comando;
	t_directorio directorio;

	// Validaciones.
	pathArchivoYamaFs = argumentos[1];
	if (!esValido(pathArchivoYamaFs)) {
		printf("La ruta '%s' no es valida.\n", pathArchivoYamaFs);
		return;
	}

	if (!existeArchivoEnYamaFs(pathArchivoYamaFs)) {
		printf("El archivo '%s' no existe.\n", pathArchivoYamaFs);
		return;
	}

	nombreArchivo = obtenerNombreArchivo(pathArchivoYamaFs);
	// Creo un archivo temporal en el fs local, reutilizando cpto.
	directorio = string_new();
	string_append(&directorio, PATH_METADATA);
	char *args[] = { "cpto", pathArchivoYamaFs, directorio };
	ejecutarCpto(args);

	pathArchivoTemporal = string_new();
	string_append(&pathArchivoTemporal, PATH_METADATA);
	string_append(&pathArchivoTemporal, "/");
	string_append(&pathArchivoTemporal, nombreArchivo);

	// Preparo el comando.
	comando = string_new();
	string_append(&comando, "md5sum ");
	string_append(&comando, pathArchivoTemporal);
	string_append(&comando, " | awk '{ print $1 }'");

	// Ejecuto la llamada a sistema.
	system(comando);

	// Libero recursos.
	remove(pathArchivoTemporal); // Borra el archivo temporal.
	free(nombreArchivo);
	free(comando);
}

void ejecutarLs(char **argumentos) {
	int i;
	bool hayDirectorios = false;
	char *path;

	path = argumentos[1];
	if (esValido(path)) {
		if (!existePathDirectorio(path)) {
			printf("El directorio '%s' no existe.\n", path);
			free(path);
			return;
		}

		int indice = obtenerIndice(path);

		// Muestro todos los archivos que estan en este directorio.
		char *comando = string_new();
		string_append(&comando, "ls ");
		string_append(&comando, PATH_METADATA);
		string_append(&comando, "/archivos/");
		string_append(&comando, string_itoa(indice));
		system(comando);
		free(comando);

		// Muestro todos los directorios que estan en este directorio.
		for (i = 0; i < CANTIDAD_DIRECTORIOS; i++) {
			if (directorios[i].padre == indice && !sonIguales(directorios[i].nombre, "")) {
				printf(ANSI_COLOR_BLUE "%s " ANSI_COLOR_RESET,
						directorios[i].nombre);
				hayDirectorios = true;
			}
		}

		if(hayDirectorios) // Un chiche :p
			puts("");

	} else {
		printf("La ruta '%s' no es valida.\n", path);
	}
	free(path);
}

void ejecutarInfo(char **argumentos) {
	int i;
	t_bloque *bloque;
	char *pathArchivo;
	t_archivo_a_persistir *archivo;

	pathArchivo = argumentos[1];
	if (!esValido(pathArchivo)) {
		printf("La ruta ingresada '%s' no es valida.\n", pathArchivo);
		return;
	}

	archivo = abrirArchivo(pathArchivo);
	if (!archivo) {
		printf("El archivo '%s' no existe.\n", pathArchivo);
		return;
	}

	// Header
	puts(
			" ---------------------------------------------------------------------------------------");
	printf("|				Info '%s'				|\n", archivo->nombreArchivo);
	puts(
			" ---------------------------------------------------------------------------------------\n");

	// Body
	printf("Tamaño: %d bytes.\n", archivo->tamanio);
	if (archivo->tipo == BINARIO) {
		printf("Tipo: binario.\n");
	} else if (archivo->tipo == TEXTO) {
		printf("Tipo: de texto.\n");
	} else {
		fprintf(stderr, "[ERRO]: el tipo del archivo no se pudo determinar.\n");
	}

	printf("Directorio padre: %d.\n", archivo->indiceDirectorio);
	printf("Cantidad de bloques: %d.\n", archivo->bloques->elements_count);
	printf(
			"\n ----------------------------------------------------------------------------------------------------");
	puts(
			"\n|     Bloque    |           Copia0              |             Copia1            |     Fin de bloque  |");
	printf(
			" ----------------------------------------------------------------------------------------------------");
	for (i = 0; i < archivo->bloques->elements_count; i++) {
		bloque = list_get(archivo->bloques, i);
		printf("\n|	%d	", bloque->numeroBloque);
		printf("|	Nodo %d - Bloque %d	", bloque->nodoCopia0->idNodo,
				bloque->numeroBloqueCopia0);
		printf("|	Nodo %d - Bloque %d	", bloque->nodoCopia1->idNodo,
				bloque->numeroBloqueCopia1);
		printf("|	%d      |\n", bloque->bytesOcupados);
		printf(
				" ---------------------------------------------------------------------------------------------------- ");
	}
	printf("\n");

	// Libero recursos.
	free(archivo->nombreArchivo);
	list_destroy_and_destroy_elements(archivo->bloques, (void*) liberarBLoque);
	free(archivo);
}

void ejecutarRename(char **argumentos) {
	char *pathOriginal, *nombreFinal;

	pathOriginal = argumentos[1];
	if (!esValido(pathOriginal)) {
		printf("La ruta '%s' no es valida.\n", pathOriginal);
		return;
	}

	nombreFinal = argumentos[2];
	if (string_starts_with(nombreFinal, "/")) {
		printf("El nombre '%s' no es valido.\n", nombreFinal);
		return;
	}

	// Determina si lo que se desea renombrar es un archivo o un directorio.
	if (existePathDirectorio(pathOriginal)) {
		renombrarDirectorio(pathOriginal, nombreFinal);
	} else {
		renombrarArchivo(pathOriginal, nombreFinal);
	}

	// Libero recursos.
	free(pathOriginal);
	free(nombreFinal);
}

void ejecutarMv(char **argumentos) {
	char *pathOriginal, *pathFinal;

	pathOriginal = argumentos[1];
	if (!esValido(pathOriginal)) {
		printf("La ruta '%s' no es valida.\n", pathOriginal);
		return;
	}

	pathFinal = argumentos[2];
	if (!esValido(pathFinal)) {
		printf("La ruta '%s' no es valida.\n", pathFinal);
		return;
	}

	// Determina si lo que hay que mover es un archivo o un directorio.
	if (existePathDirectorio(pathOriginal)) {
		moverDirectorio(pathOriginal, pathFinal);
	} else {
		moverArchivo(pathOriginal, pathFinal);
	}

	// Libero recursos.
	free(pathOriginal);
	free(pathFinal);
}

void ejecutarCpfrom(char **argumentos) {
	char tipo;
	char *pathArchivoOrigen, *pathDirectorioYamaFs, *nombreArchivo;

	pathArchivoOrigen = argumentos[1];
	pathDirectorioYamaFs = argumentos[2];

	// Validaciones.
	if (esValido(pathDirectorioYamaFs)) {
		FILE *datos = fopen(pathArchivoOrigen, "r");
		if (!datos) {
			printf("El archivo '%s' no existe.\n", pathArchivoOrigen);
			return;
		}

		if (!esArchivoRegular(pathArchivoOrigen)) {
			printf("'%s' es un directorio.\n", pathArchivoOrigen);
			return;
		}

		if (!existePathDirectorio(pathDirectorioYamaFs)) {
			printf("El directorio '%s' no existe.\n", pathDirectorioYamaFs);
			return;
		}

		// Tener en cuenta si no se guarda bien de devolver un error (si no estan conectados los datanodes ?)
		nombreArchivo = obtenerNombreArchivo(pathArchivoOrigen);
		tipo = obtenerTipo(pathArchivoOrigen);
		// CONSIDERAR ENVIAR LAS COPIAS DE CADA BLOQUE EN 2 HILOS DIFERENTES!
		almacenarArchivo(pathDirectorioYamaFs, nombreArchivo, tipo, datos);
		fclose(datos);
	} else
		printf("Los parametros ingresados: %s, %s no son validos.\n",
				pathArchivoOrigen, pathDirectorioYamaFs);
}

void ejecutarCpto(char **argumentos) {
	int i;
	char *pathArchivoYamaFs, *pathDirectorioLocalFs, *pathNuevoArchivo;
	FILE *nuevoArchivo;
	DIR *dir;
	t_bloque *bloque;
	t_archivo_a_persistir *archivo;

	// Validaciones.
	pathArchivoYamaFs = argumentos[1];
	if (!esValido(pathArchivoYamaFs)) {
		printf("La ruta '%s' no es valida.\n", pathArchivoYamaFs);
		return;
	}

	// Si el directorio no existe en el fs local.
	pathDirectorioLocalFs = argumentos[2];
	dir = opendir(pathDirectorioLocalFs);
	if (dir) {
		closedir(dir);
	} else if (ENOENT == errno) {
		printf("El directorio '%s' no existe.\n", pathDirectorioLocalFs);
		return;
	}

	// Si el archivo no existe en ese directorio de yamafs.
	if (!existeArchivoEnYamaFs(pathArchivoYamaFs)) {
		printf("El archivo '%s' no existe.\n", pathArchivoYamaFs);
		return;
	}

	// Lee el archivo en el YamaFs.
	archivo = leerArchivo(pathArchivoYamaFs);
	if (!archivo) {
		fprintf(stderr, "[ERROR]: no se pudo leer el archivo '%s'.\n",
				pathArchivoYamaFs);
		return;
	}

	// Si lo pudo leer bien escribo su contenido en un nuevo archivo en el filesystem local.
	pathNuevoArchivo = string_new();
	string_append(&pathNuevoArchivo, pathDirectorioLocalFs);
	string_append(&pathNuevoArchivo, "/");
	string_append(&pathNuevoArchivo, obtenerNombreArchivo(pathArchivoYamaFs));
	nuevoArchivo = fopen(pathNuevoArchivo, "w");
	if (!nuevoArchivo) {
		fprintf(stderr, "[ERROR]: no se pudo importar el archivo '%s'.\n",
				pathArchivoYamaFs);
		return;
	}

	for (i = 0; i < archivo->bloques->elements_count; i++) {
		bloque = list_get(archivo->bloques, i);
		fwrite(bloque->contenido, sizeof(char), bloque->bytesOcupados,
				nuevoArchivo);
	}

	// Libero recursos.
	fclose(nuevoArchivo);
	free(pathArchivoYamaFs);
	free(pathDirectorioLocalFs);
	free(pathNuevoArchivo);
}

void ejecutarCpblock(char **argumentos) {
	char *pathArchivo;
	int numeroBloque, idNodo;
	t_bloque *bloque;

	// Validaciones
	pathArchivo = argumentos[1];
	if (!esValido(pathArchivo)) {
		printf("La ruta '%s' no es valida.\n", pathArchivo);
		return;
	}

	if (!esNumero(argumentos[2])) {
		printf("El numero de bloque '%s' no es valido.\n", argumentos[2]);
		return;
	}
	numeroBloque = atoi(argumentos[2]);

	if (!esNumero(argumentos[3])) {
		printf("El id de nodo '%s' no es valido.\n", argumentos[3]);
		return;
	}
	idNodo = atoi(argumentos[3]);

	// Obtengo su contenido.
	bloque = obtenerBloque(pathArchivo, numeroBloque);
	if (!bloque)
		return;

	// Preparo el bloque a guardar...(ver si en el nodo donde va a copiarse el bloque debe existir el mismo archivo).

	// Libero recursos.
	free(pathArchivo);
}
