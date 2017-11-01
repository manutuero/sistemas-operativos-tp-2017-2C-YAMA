#include "funcionesFileSystem.h"

int main(int argc, char **argv) {
	ARCHCONFIG = "fsConfig.cfg";
	cargarArchivoDeConfiguracionFS(ARCHCONFIG);

	if (argc > 1 && sonIguales(argv[1], "--tests")) {
		return correrTests(); // Cuidado que borra el estado anterior jeje.
	} else {
		// Agregar un logger (cuando estemos por terminar).
		if (hayEstadoAnterior()) {
			archivos = list_create();
			restaurarEstructurasAdministrativas();
			puts(
					"Hay estado anterior.. para las pruebas borrar la carpeta metadata"); // borrar luego :p
		} else {
			crearDirectorioMetadata();
			puts("No hay estado anterior. Esperando datanodes..."); // borrar luego :p
		}

		// Espera que se conecten todos los datanodes para alcanzar un estado "estable", no dejar entrar a YAMA hasta que eso pase.
		// es  un hilo ya que una vez que queda estable, sigue escuchando conexiones
		pthread_t hiloConexiones;
		pthread_create(&hiloConexiones, NULL, esperarConexionesDatanodes, NULL);

		/* Iniciar hilo consola. Habria que poner un mutex o algo para evitar que entre mientras el fs no sea estable.
		 Tambien agregar hilo de conexion con Yama. */
		pthread_t hiloConsola;
		pthread_create(&hiloConsola, NULL, levantarConsola, NULL);

		pthread_join(hiloConexiones, NULL);
		pthread_join(hiloConsola, NULL);
		// Liberar la estructura de config pasandola como puntero...
		return 0;
	}
}
