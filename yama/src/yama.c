#include "funcionesYAMA.h"

int main(void) {
	cargarArchivoDeConfiguracion();
	conectarseAFS();
	yamaEscuchando();
	// escucharMasters();
	while (1) {
	}

	return EXIT_SUCCESS;
}
