#include "datanodeAPI.h"

void cargarArchivoConfiguracion(char* nombreArchivo) {
	char cwd[1024]; // Variable donde voy a guardar el path absoluto
	char * pathArchConfig = string_from_format("%s/%s",
			getcwd(cwd, sizeof(cwd)), nombreArchivo); // String que va a tener el path absoluto para pasarle al config_create
	t_config *config = config_create(pathArchConfig);

	if (config_has_property(config, "PUERTO_FILESYSTEM")) {
		PUERTO_FILESYSTEM = config_get_int_value(config, "PUERTO_FILESYSTEM");
	}
	if (config_has_property(config, "IP_FILESYSTEM")) {
		IP_FILESYSTEM = config_get_string_value(config, "IP_FILESYSTEM");
	}
	if (config_has_property(config, "ID_NODO")) {
		ID_NODO = config_get_string_value(config, "ID_NODO");
	}
	if (config_has_property(config, "PUERTO_WORKER")) {
		PUERTO_WORKER = config_get_int_value(config, "PUERTO_WORKER");
	}
	if (config_has_property(config, "RUTA_DATABIN")) {
		RUTA_DATABIN = config_get_string_value(config, "RUTA_DATABIN");
	}
}

void setBloque(int numero, char* datos) {
	int fileDescriptor;
	size_t bytesAEscribir = strlen(datos), tamanioArchivo;
	off_t desplazamiento = numero * UN_BLOQUE;
	char *regionDeMapeo;

	FILE *filePointer = fopen(RUTA_DATABIN, "r+");

	// Valida que el archivo exista. Caso contrario lanza error.
	if (!filePointer) {
		perror(
				"[ERROR]. El archivo 'data.bin' no existe en la ruta especificada.");
		exit(EXIT_FAILURE);
	}

	// Estructura que contiene informacion del estado de archivos y dispositivos.
	struct stat st;
	stat(RUTA_DATABIN, &st);
	tamanioArchivo = st.st_size;

	// Valida que se disponga del tamaño suficiente para realizar la escritura.
	if (tamanioArchivo < bytesAEscribir) {
		perror(
				"[ERROR]. No se dispone de tamaño suficiente en la base de datos.");
		exit(EXIT_FAILURE);
	}

	fileDescriptor = fileno(filePointer);

	if (desplazamiento >= tamanioArchivo) {
		perror("[ERROR]. El desplazamiento se paso del EOF.\n");
		exit(EXIT_FAILURE);
	}

	if (desplazamiento + bytesAEscribir > tamanioArchivo)
		bytesAEscribir = tamanioArchivo - desplazamiento;
	/* No puedo mostrar bytes pasando el EOF */

	regionDeMapeo = mmap(NULL, bytesAEscribir,
	PROT_READ | PROT_WRITE,
	MAP_SHARED, fileDescriptor, desplazamiento);

	if (regionDeMapeo == MAP_FAILED) {
		perror("[ERROR]. No se pudo reservar la region de mapeo.");
		exit(EXIT_FAILURE);
	}

	strncpy(regionDeMapeo, datos, bytesAEscribir);

	// Libero la region de mapeo solicitada.
	munmap(regionDeMapeo, bytesAEscribir);
	fclose(filePointer);
}

char* getBloque(int numero) {
	int fileDescriptor;
	size_t bytesALeer = UN_BLOQUE, tamanioArchivo;
	off_t desplazamiento = numero * UN_BLOQUE;
	char *regionDeMapeo, *data = malloc(bytesALeer);

	FILE *filePointer = fopen(RUTA_DATABIN, "r");

	// Valida que el archivo exista. Caso contrario lanza error.
	if (!filePointer) {
		perror(
				"[ERROR]. El archivo 'data.bin' no existe en la ruta especificada.");
		exit(EXIT_FAILURE);
	}

	// Estructura que contiene informacion del estado de archivos y dispositivos.
	struct stat st;
	stat(RUTA_DATABIN, &st);
	tamanioArchivo = st.st_size;

	if (desplazamiento >= tamanioArchivo) {
		perror("[ERROR]. El desplazamiento se paso del EOF.\n");
		exit(EXIT_FAILURE);
	}

	if (desplazamiento + bytesALeer > tamanioArchivo)
		bytesALeer = tamanioArchivo - desplazamiento;
	/* No puedo mostrar bytes pasando el EOF */

	fileDescriptor = fileno(filePointer);

	regionDeMapeo = mmap(NULL, bytesALeer,
	PROT_READ, MAP_SHARED, fileDescriptor, desplazamiento);

	if (regionDeMapeo == MAP_FAILED) {
		perror("[ERROR]. No se pudo reservar la region de mapeo.");
		exit(EXIT_FAILURE);
	}

	strncpy(data, regionDeMapeo, bytesALeer);

	// Libero la region de mapeo solicitada.
	munmap(regionDeMapeo, bytesALeer);
	fclose(filePointer);

	return data;
}
