

enum headers {SOLICITUD_EJECUTAR_COMANDO_CONSOLA = 1};

typedef struct{
	int funcion;
	int opcion;
	char *parametro1;
	char *parametro2;
	int bloque;
	int idNodo;
	} comando;
typedef struct{
	int id;
	int tamanio;

} header;
/* Recibe un mensaje serializado y devuelve un puntero generico (void) al buffer de memoria donde estara la respuesta deserializada del mensaje. */
void* deserializar(void* mensaje, header header);

void* deserializarSolicitudEjecutarComando(void*);

int recibirPorSocket(int, void *, int);
int recibirHeader(int, header*);
void * recibirPaquete(int, header);
