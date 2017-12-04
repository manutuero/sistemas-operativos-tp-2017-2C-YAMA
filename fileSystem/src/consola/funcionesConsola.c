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

void ejecutarFormat() {
	if (nodos->elements_count >= 2) {
		// Inicio el fs por primera vez o con el flag --clean.
		if (estadoFs == NO_ESTABLE) {
			crearDirectorioMetadata();
			persistirTablaDeNodos();
			persistirBitmaps();
			estadoFs = ESTABLE;
			estadoNodos = ACEPTANDO_NODOS_YA_CONECTADOS;
		} else if (estadoFs == ESTABLE) { // Ejecute format desde un estado estable
			reiniciarEstructuras();
		}

		printf("Formateando sistema... hecho.\n");
		cantidad_nodos_esperados = nodos->elements_count;
		sem_post(&semEstadoEstable);
	} else {
		printf(
				"No se puede formatear el sistema, se necesitan al menos 2 nodos conectados.\n");
	}
}

void ejecutarRmArchivo(char **argumentos) {
	int i, idNodoCopia0, idNodoCopia1, nroBloqueDataBin;
	char *pathArchivo, *comando;
	t_archivo_a_persistir *archivo;
	t_list *bloques;
	t_bloque *bloque;

	// Validaciones
	pathArchivo = argumentos[1];
	if (!esValido(pathArchivo)) {
		printf("La ruta '%s' no es valida.\n", pathArchivo);
		free(pathArchivo);
		return;
	}

	if (!existeArchivoEnYamaFs(pathArchivo)) {
		printf("El archivo '%s' no existe.\n", pathArchivo);
		free(pathArchivo);
		return;
	}

	// Libero los bloques del bitmap de cada nodo que almacena los bloques del archivo.
	archivo = abrirArchivo(pathArchivo);
	bloques = archivo->bloques;
	for (i = 0; i < bloques->elements_count; i++) {
		bloque = list_get(bloques, i);

		idNodoCopia0 = bloque->nodoCopia0->idNodo;
		nroBloqueDataBin = bloque->numeroBloqueCopia0;
		liberarBloqueBitmaps(idNodoCopia0, nroBloqueDataBin);
		free(bloque->nodoCopia0);

		idNodoCopia1 = bloque->nodoCopia1->idNodo;
		nroBloqueDataBin = bloque->numeroBloqueCopia1;
		liberarBloqueBitmaps(idNodoCopia1, nroBloqueDataBin);
		free(bloque->nodoCopia1);

		free(bloque);
	}

	comando = string_new();
	string_append(&comando, "rm ");
	string_append(&comando, PATH_METADATA);
	string_append(&comando, "/archivos/");
	string_append(&comando, string_itoa(archivo->indiceDirectorio));
	string_append(&comando, "/");
	string_append(&comando, obtenerNombreArchivo(pathArchivo));
	system(comando);

	// Libero recursos
	list_destroy(bloques);
	free(archivo->nombreArchivo);
	free(archivo);
	free(pathArchivo);
	free(comando);
}

void ejecutarRmDirectorio(char **argumentos) {
	int i, indiceDirectorio, numeroResultados, cantidadArchivos = 0;
	char *pathDirectorioYama, *opcion, *pathDirectorioLocal, *comando;
	t_directorio directorio;
	struct dirent **resultados = NULL;

	// Validaciones
	pathDirectorioYama = argumentos[2];
	if (!esValido(pathDirectorioYama)) {
		printf("La ruta '%s' no es valida.\n", pathDirectorioYama);
		free(pathDirectorioYama);
		return;
	}

	opcion = argumentos[1];
	if (!sonIguales(opcion, "-d")) {
		printf("La opcion '%s' no es valida.\n", opcion);
		free(pathDirectorioYama);
		free(opcion);
		return;
	}

	if (sonIguales(pathDirectorioYama, "/root")) {
		printf("No se puede eliminar el directorio raiz...abortando.\n");
		free(pathDirectorioYama);
		free(opcion);
		return;
	}

	if (!existePathDirectorio(pathDirectorioYama)) {
		printf("El directorio '%s' no existe.\n", pathDirectorioYama);
		free(pathDirectorioYama);
		free(opcion);
		return;
	}

	// Preparo la ruta del directorio a borrar.
	indiceDirectorio = obtenerIndice(pathDirectorioYama);

	pathDirectorioLocal = string_new();
	string_append(&pathDirectorioLocal, PATH_METADATA);
	string_append(&pathDirectorioLocal, "/archivos/");
	string_append(&pathDirectorioLocal, string_itoa(indiceDirectorio));

	// Verifico si el directorio no esta vacio
	numeroResultados = scandir(pathDirectorioLocal, &resultados, NULL,
			alphasort);
	for (i = 2; i < numeroResultados; i++)
		cantidadArchivos++;

	directorio = directorios[indiceDirectorio].nombre;
	if (cantidadArchivos > 0 || esDirectorioPadre(indiceDirectorio)) {
		printf("El directorio '%s' no esta vacio...abortando.\n",
				pathDirectorioYama);
		return;
	} else {
		memset(directorio, 0, strlen(directorio));
		directorios[indiceDirectorio].padre = 0; // Todos los directorios son hijos de root.

		comando = string_new();
		string_append(&comando, "rmdir ");
		string_append(&comando, pathDirectorioLocal);
		system(comando);

		// Libero recursos
		free(comando);
		free(pathDirectorioYama);
		free(opcion);
	}
}

void ejecutarRmBloque(char **argumentos) {
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
}

void ejecutarCat(char **argumentos) {
	int i;
	char *pathArchivo, *contenido;
	t_list *bloques;
	off_t offset = 0;
	t_bloque *bloque;
	t_archivo_a_persistir *archivo;

	// Validaciones.
	pathArchivo = argumentos[1];
	if (!esValido(pathArchivo)) {
		printf("La ruta '%s' no es valida.\n", pathArchivo);
		free(pathArchivo);
		return;
	}

	if(!existeArchivoEnYamaFs(pathArchivo)) {
		printf("El archivo '%s' no existe.\n", pathArchivo);
		free(pathArchivo);
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
	liberarArchivoYNodos(archivo);
}

void ejecutarMkdir(char **argumentos) {
	char *path = argumentos[1];
	if (esValido(path) && strlen(path) > 1) {
		mkdirFs(path);
	} else
		printf(
				"mkdir: no se puede crear el directorio «%s»: La ruta ingresada no es valida.\n",
				path);
	free(path);
}

void ejecutarMd5(char **argumentos) {
	char *pathArchivoYamaFs, *nombreArchivo, *pathArchivoTemporal, *comando;
	t_directorio directorio;

	// Validaciones.
	pathArchivoYamaFs = argumentos[1];
	if (!esValido(pathArchivoYamaFs)) {
		printf("La ruta '%s' no es valida.\n", pathArchivoYamaFs);
		free(pathArchivoYamaFs);
		return;
	}

	if (!existeArchivoEnYamaFs(pathArchivoYamaFs)) {
		printf("El archivo '%s' no existe.\n", pathArchivoYamaFs);
		free(pathArchivoYamaFs);
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

	if (access(pathArchivoTemporal, F_OK) != -1) {
		// Preparo el comando.
		comando = string_new();
		string_append(&comando, "md5sum ");
		string_append(&comando, pathArchivoTemporal);
		string_append(&comando, " | awk '{ print $1 }'");

		// Ejecuto la llamada a sistema.
		system(comando);

		remove(pathArchivoTemporal); // Borra el archivo temporal.
		free(comando);
	}

	// Libero recursos.
	free(nombreArchivo);
	free(pathArchivoTemporal);
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
			if (directorios[i].padre == indice
					&& !sonIguales(directorios[i].nombre, "")) {
				printf(ANSI_COLOR_BLUE "%s " ANSI_COLOR_RESET,
						directorios[i].nombre);
				hayDirectorios = true;
			}
		}

		if (hayDirectorios) // Un chiche :p
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
		free(pathArchivo);
		return;
	}

	archivo = abrirArchivo(pathArchivo);
	if (!archivo) {
		printf("El archivo '%s' no existe.\n", pathArchivo);
		free(pathArchivo);
		return;
	}

	// Header
	puts("\n -----------------------------------------------");
	printf("|		Info '%s'		|\n", archivo->nombreArchivo);
	puts(" -----------------------------------------------\n");

	// Body
	printf("Tamaño: %d bytes.\n", archivo->tamanio);
	if (archivo->tipo == BINARIO) {
		printf("Tipo: binario.\n");
	} else if (archivo->tipo == TEXTO) {
		printf("Tipo: de texto.\n");
	} else {
		fprintf(stderr,
				"[ERROR]: el tipo del archivo no se pudo determinar.\n");
	}

	printf("Directorio padre: %d.\n", archivo->indiceDirectorio);
	printf("Cantidad de bloques: %d.\n", archivo->bloques->elements_count);
	printf(
			"\n -----------------------------------------------------------------------");
	puts(
			"\n|   Bloque  |        Copia0       |        Copia1       | Fin de bloque |");
	printf(
			" -----------------------------------------------------------------------");
	for (i = 0; i < archivo->bloques->elements_count; i++) {
		bloque = list_get(archivo->bloques, i);
		printf("\n|     %d     ", bloque->numeroBloque);
		printf("|  Nodo %d - Bloque %d  ", bloque->nodoCopia0->idNodo,
				bloque->numeroBloqueCopia0);
		printf("|  Nodo %d - Bloque %d  ", bloque->nodoCopia1->idNodo,
				bloque->numeroBloqueCopia1);
		printf("|     %d   |\n", bloque->bytesOcupados);
		printf(
				" -----------------------------------------------------------------------");
	}
	printf("\n");

	// Libero recursos.
	liberarArchivoSinContenido(archivo);
	free(pathArchivo);
}

void ejecutarRename(char **argumentos) {
	char *pathOriginal, *nombreFinal;

	pathOriginal = argumentos[1];
	if (!esValido(pathOriginal)) {
		printf("La ruta '%s' no es valida.\n", pathOriginal);
		free(pathOriginal);
		return;
	}

	nombreFinal = argumentos[2];
	if (string_starts_with(nombreFinal, "/")) {
		printf("El nombre '%s' no es valido.\n", nombreFinal);
		free(pathOriginal);
		free(nombreFinal);
		return;
	}

	// Determina si lo que se desea renombrar es un archivo o un directorio.
	if (existePathDirectorio(pathOriginal)) {
		if(sonIguales(pathOriginal, "/root") || sonIguales(pathOriginal, "/root/")) {
			printf("El directorio 'root' no puede ser renombrado.\n");
			free(pathOriginal);
			free(nombreFinal);
			return;
		}
		renombrarDirectorio(pathOriginal, nombreFinal);
	} else {
		if(!existeArchivoEnYamaFs(pathOriginal)) {
			printf("El archivo '%s' no existe en ':yamafs'.\n", pathOriginal);
			free(pathOriginal);
			free(nombreFinal);
			return;
		}
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
		free(pathOriginal);
		return;
	}

	pathFinal = argumentos[2];
	if (!esValido(pathFinal)) {
		printf("La ruta '%s' no es valida.\n", pathFinal);
		free(pathOriginal);
		free(pathFinal);
		return;
	}

	// Determina si lo que hay que mover es un archivo o un directorio.
	if (existePathDirectorio(pathOriginal)) {
		if(sonIguales(pathOriginal, "/root")) {
			printf("El directorio 'root' no puede ser reubicado.\n");
			free(pathOriginal);
			free(pathFinal);
			return;
		}

		moverDirectorio(pathOriginal, pathFinal);
	} else {
		if(!existeArchivoEnYamaFs(pathOriginal)) {
			printf("El archivo '%s' no existe en ':yamafs'.\n", pathOriginal);
			free(pathOriginal);
			free(pathFinal);
			return;
		}
		moverArchivo(pathOriginal, pathFinal);
	}

	// Libero recursos.
	free(pathOriginal);
	free(pathFinal);
}

void ejecutarCpfrom(char **argumentos) {
	char tipo;
	char *pathArchivoOrigen, *pathDirectorioYamaFs, *nombreArchivo;
	FILE *datos;

	pathArchivoOrigen = argumentos[1];
	pathDirectorioYamaFs = argumentos[2];

	// Validaciones.
	if (!esValido(pathDirectorioYamaFs)) {
		printf("La ruta '%s' no es valida.\n", pathDirectorioYamaFs);
		free(pathArchivoOrigen);
		free(pathDirectorioYamaFs);
		return;
	}

	datos = fopen(pathArchivoOrigen, "r");
	if (!datos) {
		printf("El archivo '%s' no existe en su filesystem (ext4).\n", pathArchivoOrigen);
		free(pathArchivoOrigen);
		free(pathDirectorioYamaFs);
		return;
	}

	if (!esArchivoRegular(pathArchivoOrigen)) {
		printf("'%s' es un directorio.\n", pathArchivoOrigen);
		fclose(datos);
		free(pathArchivoOrigen);
		free(pathDirectorioYamaFs);
		return;
	}

	if (!existePathDirectorio(pathDirectorioYamaFs)) {
		printf("El directorio '%s' no existe en ':yamafs'.\n", pathDirectorioYamaFs);
		fclose(datos);
		free(pathArchivoOrigen);
		free(pathDirectorioYamaFs);
		return;
	}

	nombreArchivo = obtenerNombreArchivo(pathArchivoOrigen);
	tipo = obtenerTipo(pathArchivoOrigen);
	almacenarArchivo(pathDirectorioYamaFs, nombreArchivo, tipo, datos);

	// Libero recursos.
	fclose(datos);
	free(pathArchivoOrigen);
	free(pathDirectorioYamaFs);
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
		free(pathArchivoYamaFs);
		return;
	}

	// Si el directorio no existe en el fs local.
	pathDirectorioLocalFs = argumentos[2];
	dir = opendir(pathDirectorioLocalFs);
	if (dir) {
		closedir(dir);
	} else if (ENOENT == errno) {
		printf("El directorio '%s' no existe en su filesystem (ext4).\n",
				pathDirectorioLocalFs);
		free(pathArchivoYamaFs);
		free(pathDirectorioLocalFs);
		return;
	}

	// Si el archivo no existe en ese directorio de yamafs.
	if (!existeArchivoEnYamaFs(pathArchivoYamaFs)) {
		printf("El archivo '%s' no existe en ':yamafs'.\n", pathArchivoYamaFs);
		free(pathArchivoYamaFs);
		free(pathDirectorioLocalFs);
		return;
	}

	// Lee el archivo en el YamaFs.
	archivo = leerArchivo(pathArchivoYamaFs);
	if (!archivo) {
		fprintf(stderr, "[ERROR]: no se pudo leer el archivo '%s'.\n",
				pathArchivoYamaFs);
		free(pathArchivoYamaFs);
		free(pathDirectorioLocalFs);
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
		free(pathArchivoYamaFs);
		free(pathDirectorioLocalFs);
		return;
	}

	// Escribo en el nuevo archivo.
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
	liberarArchivoYNodos(archivo);
}

void ejecutarCpblock(char **argumentos) {
	int i, numeroBloque, idNodo, resultado, tamanioBitmap, indice,
			cantidadClaves, cantidadCopias = 0;
	char *pathArchivo, *nombreArchivo, *pathMetadataArchivo, *clave, *valor,
			*comando;
	t_directorio directorio;
	t_config *diccionario;
	bool nodoConectado = false;
	t_bitmap bitmap;
	t_nodo *nodo;
	t_bloque *bloque;

	// Validaciones
	pathArchivo = argumentos[1];
	if (!esValido(pathArchivo)) {
		printf("La ruta '%s' no es valida.\n", pathArchivo);
		free(pathArchivo);
		return;
	}

	if (!esNumero(argumentos[2])) {
		printf("El numero de bloque '%s' no es valido.\n", argumentos[2]);
		free(pathArchivo);
		free(argumentos[2]);
		return;
	}
	numeroBloque = atoi(argumentos[2]);

	if (!esNumero(argumentos[3])) {
		printf("El id de nodo '%s' no es valido.\n", argumentos[3]);
		free(pathArchivo);
		free(argumentos[2]);
		free(argumentos[3]);
		return;
	}
	idNodo = atoi(argumentos[3]);

	// Busca el nodo con 'idNodo' en la lista de nodos conectados.
	for (i = 0; i < nodos->elements_count; i++) {
		nodo = list_get(nodos, i);
		if (nodo->idNodo == idNodo) {
			nodoConectado = true;
			bitmap = nodo->bitmap;
			break;
		}
	}

	// Sino lo encuentra conectado devuelve un flag de error.
	if (!nodoConectado) {
		printf("El nodo id '%d' no se encuentra conectado.\n", idNodo);
		free(pathArchivo);
		free(argumentos[2]);
		free(argumentos[3]);
		return;
	}

	// Si esta conectado obtengo el bloque.
	bloque = obtenerBloque(pathArchivo, numeroBloque);
	if (!bloque) {
		free(pathArchivo);
		free(argumentos[2]);
		free(argumentos[3]);
		liberarBloqueSinContenido(bloque);
		return;
	}

	// Reutilizo el campo nodoCopia0 (por como esta hecha la funcion guardarBloqueEnNodo).
	// Solicito al bitmap del nodo un bloque libre.
	bloque->nodoCopia0 = nodo;
	tamanioBitmap = strlen(bitmap);
	bloque->numeroBloqueCopia0 = obtenerYReservarBloqueBitmap(bitmap,
			tamanioBitmap);

	// Escribo el bloque en el nodo solicitado.
	resultado = guardarBloque(bloque, idNodo);
	if (resultado != GUARDO_BLOQUE_OK) {
		free(pathArchivo);
		free(argumentos[2]);
		free(argumentos[3]);
		liberarBloqueSinContenidoYNodos(bloque);
		return;
	}

	actualizarBitmaps();

	// Si salio ok, actualiza la metadata del archivo agregando la metadata del bloque copiado.
	nombreArchivo = obtenerNombreArchivo(pathArchivo);
	directorio = obtenerPathDirectorio(pathArchivo);
	indice = obtenerIndice(directorio);

	pathMetadataArchivo = string_new();
	string_append(&pathMetadataArchivo, PATH_METADATA);
	string_append(&pathMetadataArchivo, "/archivos/");
	string_append(&pathMetadataArchivo, string_itoa(indice));
	string_append(&pathMetadataArchivo, "/");
	string_append(&pathMetadataArchivo, nombreArchivo);

	diccionario = config_create(pathMetadataArchivo);
	if (!diccionario) {
		fprintf(stderr, "[ERROR]: no se pudo abrir la metadata del archivo.\n");
		free(pathArchivo);
		free(argumentos[2]);
		free(argumentos[3]);
		liberarBloqueSinContenidoYNodos(bloque);
		return;
	}

	// Averiguo cuantas copias de 'numeroBloque' de este archivo hay en el sistema, para agregar la siguiente al archivo.
	cantidadClaves = config_keys_amount(diccionario);
	for (i = 0; i < cantidadClaves; i++) {
		clave = string_new();
		string_append(&clave, "BLOQUE");
		string_append(&clave, string_itoa(numeroBloque));
		string_append(&clave, "COPIA");
		string_append(&clave, string_itoa(i));

		if (config_has_property(diccionario, clave)) {
			cantidadCopias++;
		} else {
			break;
		}

		free(clave);
	}

	// Escribo la nueva copia en el archivo.
	valor = string_new();
	string_append(&valor, "[");
	string_append(&valor, string_itoa(bloque->nodoCopia0->idNodo));
	string_append(&valor, ",");
	string_append(&valor, string_itoa(bloque->numeroBloqueCopia0));
	string_append(&valor, "]");

	config_set_value(diccionario, clave, valor);
	config_save(diccionario);

	// Reordeno el diccionario llamando a un script de la carpeta thePonchos.
	comando = string_new();
	string_append(&comando, "/home/utnso/thePonchos/ordenarArchivo.sh ");
	string_append(&comando, pathMetadataArchivo);
	system(comando);

	// Libero recursos.
	liberarBloqueSinContenidoYNodos(bloque);
	config_destroy(diccionario);
	free(clave);
	free(valor);
	free(pathArchivo);
	free(nombreArchivo);
	free(directorio);
	free(comando);
	free(pathMetadataArchivo);
}
