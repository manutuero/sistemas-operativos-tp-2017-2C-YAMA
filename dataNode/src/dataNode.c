#include "funcionesDataNode.h"

int main(void) {
	int socketFs;

	crearLoggerDatanode();
	NODOARCHCONFIG = "nodoConfig.cfg";
	cargarArchivoConfiguracionDatanode(NODOARCHCONFIG);
	abrirDatabin();

	socketFs = conectarAfilesystem(IP_FILESYSTEM, PUERTO_FILESYSTEM);
	if (socketFs == 0) {
		log_error(logger, "No se pudo conectar al FileSystem.");
	} else {
		log_info(logger, "Conectado al FileSystem con socket n°: %d.", socketFs);
	}

	// Escuchar peticiones FileSystem (get y set)
	if (socketFs != 0) {
		cerrarSocket(socketFs);
	}

	cerrarDatabin();
	return 0;
}
