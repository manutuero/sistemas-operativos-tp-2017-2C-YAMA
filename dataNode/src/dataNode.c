#include "funcionesDataNode.h"

int main(void) {
	int socketFs;

	NODOARCHCONFIG = "nodoConfig.cfg";
	cargarArchivoConfiguracion(NODOARCHCONFIG);
	socketFs = conectarAfilesystem(IP_FILESYSTEM, PUERTO_FILESYSTEM);

	if (socketFs == 0) {
		printf("ERROR AL CONECTAR FileSystem.\n");
	} else {
		printf("Me conecte al FileSystem con socket: %d.\n", socketFs);

	}

	//Escuchar peticiones FileSystem

	if (socketFs != 0) {
		cerrarSocket(socketFs);
	}
	return 0;
}
