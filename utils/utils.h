enum headers {SOLICITUD_EJECUTAR_COMANDO_CONSOLA = 1};

/* Recibe un mensaje serializado y devuelve un puntero generico (void) al buffer de memoria donde estara la respuesta deserializada del mensaje. */
void* deserializar(void* mensaje, header header);

void* deserializarSolicitudEjecutarComando(mensaje);
