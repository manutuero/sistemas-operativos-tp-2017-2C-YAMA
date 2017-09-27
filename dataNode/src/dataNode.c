#include "funcionesDataNode.h"

int main(void) {
	int socketFs;

	NODOARCHCONFIG = "nodoConfig.cfg";
	cargarArchivoConfiguracion(NODOARCHCONFIG);
	socketFs = conectarAfilesystem(IP_FILESYSTEM, PUERTO_FILESYSTEM);

	if(socketFs == 0) {
		printf("Volvi al main con socketFs igual a 0.\n");
	} else {
		printf("Me conecte al fs con socket: %d.\n", socketFs);
		cerrarSocket(socketFs);
	}

	return 0;
}
