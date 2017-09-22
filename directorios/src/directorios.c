#include "funcionesDirectorios.h"

int main(void) {
	char *path = "/home/utnso/workspace/directorios";

	validarMetadata(path);
	persistirDirectorios(directoriosAGuardar, path);
	obtenerDirectorios(directoriosGuardados, path);
	mostrar(directoriosGuardados);

	return EXIT_SUCCESS;
}
