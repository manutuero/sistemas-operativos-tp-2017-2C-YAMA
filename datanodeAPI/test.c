#include "datanodeAPI.h"

int main(void) {
	NODOARCHCONFIG = "nodoConfig.cfg";
	cargarArchivoConfiguracion(NODOARCHCONFIG);

	// El numero de bloque del archivo 'data.bin' y la informacion que se desea escribir en el mismo.
	int numero = 0;
	char* data = "hola mundo!!";
	char* buffer;

	setBloque(numero, data);
	buffer = getBloque(numero);
	printf("Bloque: %d\nData: %s\n", numero, buffer);

	free(buffer);
	return EXIT_SUCCESS;
}
