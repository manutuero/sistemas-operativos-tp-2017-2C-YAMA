#include "funcionesDataNode.h"

int main(void) {
	int socketFs = 0;
	NODOARCHCONFIG = "nodoConfig.cfg";
	cargarArchivoConfiguracionDatanode(NODOARCHCONFIG);
	abrirDatabin();
	// Agregar while para que cuando se desconecte el fs quede esperando conexiones nuevamente.
	socketFs = conectarAfilesystem(IP_FILESYSTEM, PUERTO_FILESYSTEM);
	if (socketFs == 0) {
		perror("No se pudo conectar al FileSystem.");
	} else {
		printf("Conectado al FileSystem con socket n°: %d.\n", socketFs);
		escucharFileSystem(socketFs);
	}
	// Escuchar peticiones FileSystem (get y set)
	if (socketFs != 0) {
		cerrarSocket(socketFs);
	}

	cerrarDatabin();
	return 0;
}
