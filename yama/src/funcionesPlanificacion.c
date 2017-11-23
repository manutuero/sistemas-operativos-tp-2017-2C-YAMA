#include "funcionesPlanificacion.h"

t_worker_Disponibles workers[30];

int algoritmo = 1;//idem anterior, 0 para clock y 1 para wclock
//uint32_t job = 0;
int idMaster = 1;

void mostrarTablaDeEstados(int i) {
	printf("Tabla de estados:\n");
	printf("job\tmaster\tnodo\tbloque\tetapa\t\testado\t\ttemporal\n");

	char* etapas[3] = {"transformacion", "reduccion local", "reduccion global"};
	char* estados[3] = {"en Proceso", "Error", "Ok"};

	for (i = 0; i < list_size(listaTablaEstados); i++) {
		t_tabla_estados registro = *(t_tabla_estados*) list_get(
				listaTablaEstados, i);
		printf("%d\t%d\t%d\t%d\t%s\t%s\t%s\n", registro.job, registro.master,
				registro.nodo, registro.bloque, etapas[registro.etapa-1], estados[registro.estado-1],
				registro.archivoTemp);
	}
}

//esta funcion va a recibir por parametro una estructura que tenga el job, el idMaster y el socket de ese master.
void *preplanificarJob(t_job* jobMaster){

	t_header header;
	t_bloqueRecv* bloqueRecibido;
	int nodo,i,cantNodosInvolucrados,*clock,*clockAux,transformacionesOk=0;
	void* buffer;

	/* envio de nombres de archivos a FS*/
	/* Reservar memoria para los clocks */

	clock = malloc(sizeof(uint32_t));
	clockAux = malloc(sizeof(uint32_t));

	/* Recibir de FS la composicion completa del archivo, en el header usamos
	 * tamaño de payload para saber cantidad de bloques a recibir*/

	recibirHeader(socketFS,&header);

	/* carga local de bloques de prueba */
	/*t_bloqueRecv bloques[4] = {{0, 1, 0, 3, 4},
								{1, 2, 4, 3, 9},////////////eliminar
								{2, 4, 7, 1, 3},////////////eliminar
								{3, 1, 9, 3, 11}};
	header.tamanioPayload = 4;*/


	crearListas();


	for (i=0;i<header.tamanioPayload;i++)
	{
		//Cambio bloqueRecibido por buffer en el malloc
		buffer = malloc(sizeof(t_bloqueRecv));//Se liberara cuando se destruya la lista
		bloqueRecibido = malloc(sizeof(t_bloqueRecv));//Se liberara cuando se destruya la lista
		recibirPorSocket(socketFS, buffer, sizeof(t_bloqueRecv));
		bloqueRecibido=(t_bloqueRecv*)buffer;

		//prueba local (sin sockets)
		//bloqueRecibido=&bloques[i];
		guardarEnBloqueRecibidos(bloqueRecibido);

	}
	/*              ACTUALIZAR CONFIG POR  INCREMENTO DE JOB    */
	actualizarConfig();


	t_bloqueRecv* bloque;
	for(i=0;i<list_size(listaBloquesRecibidos);i++){
			bloque = (t_bloqueRecv*)list_get(listaBloquesRecibidos,i);
			printf("bloque  %d  nodo0: %d    nodo1: %d\n",bloque->nroBloqueArch, bloque->idNodo0, bloque->idNodo1);
		}

	printf("nodos involucrados: \n");
		for(i=0;i<list_size(listaNodosInvolucrados);i++){
			nodo = *(int*)list_get(listaNodosInvolucrados,i);
			printf("nodo %d\n",*(int*)list_get(listaNodosInvolucrados,i));
		}

	/* Ordenar lista por mayor disponibilidad */
	list_sort(listaNodosInvolucrados,ordenarPorDisponibilidad);

	/* Generar vector que sera utilizado por el clock */
	cantNodosInvolucrados= list_size(listaNodosInvolucrados);
	int nodosInvolucrados[cantNodosInvolucrados];
	cargarVector(nodosInvolucrados,listaNodosInvolucrados);

	printf("nodos ordenados: \n");
			for(i=0;i<cantNodosInvolucrados;i++){
				printf("nodo %d\n",nodosInvolucrados[i]);
			}


	/* Como esta ordenado por mayor disponibilidad, clock y clockAux apuntan a 0 */

	*clock = 0;
	*clockAux = 0;

									/* Inicia la planificacion */

	/* pre-planificacion de transformaciones */
	for(i=0;i<list_size(listaBloquesRecibidos);i++)
	{
		bloqueRecibido = list_get(listaBloquesRecibidos,i);
		transformacionesOk = planificarTransformaciones(cantNodosInvolucrados,nodosInvolucrados,bloqueRecibido,clock,clockAux);
		if (transformacionesOk == 1)
			break;
	}

	if (transformacionesOk == 0)
	{
		/* pre-planificacion de reducciones locales */
		planificacionReduccionLocal();

		/* pre-planificacion de reducion global */
		planificacionReduccionGlobal(cantNodosInvolucrados,nodosInvolucrados);

		printf("planifico. \n");
		mostrarTablaDeEstados(i);

		/* Actualizar workload aplicando ecuacion*/
		actualizarWorkload(cantNodosInvolucrados,nodosInvolucrados);

		/* Enviar toda la planificacion a master */
		enviarPlanificacionAMaster(jobMaster);
	}
	else
	{
		printf("Fallo en la planificacion abortar job\n");
		restaurarWorkload();
		enviarMensajeFalloOperacion(jobMaster);
	}

	/* liberar listas para la siguiente planificacion */
	destruir_listas();

	/* Liberar variables dinamicas */
	free(buffer);
	free(clock);
	free(clockAux);

	return((void*)0);
}


/*						Descargar workload al finalizar un trabajo             */
//Esta Funcion recibira el nombreTMP del archivo que produjo el fin del trabajo
void *descargarWorkload(void *nombreTMP)
{
	t_tabla_estados *registro;
	char *nombre = (char*)nombreTMP;

	t_list *listaFiltrada;

	listaFiltrada = list_create();

	registro = encontrarRegistro(nombre);

	if(registro->job!=-1)
	{
		filtrarLista(listaFiltrada,registro->job);

		restarJob(listaFiltrada);

		list_destroy(listaFiltrada);
	}else{
		printf("No se encontro el registro\n");
		free(registro);
	}
	return((void*)0);
}


t_tabla_estados* encontrarRegistro(char *nombreTMP)
{
	int i;
	t_tabla_estados *registro;

	for(i=0;i<list_size(listaTablaEstados);i++)
	{
		registro = list_get(listaTablaEstados,i);
		if (sonIguales(nombreTMP,registro->archivoTemp))
			return registro;
	}
	registro = malloc(sizeof(t_tabla_estados));
	registro->job=-1;
	return registro;
}

void filtrarLista(t_list *listaFiltrada,int job)
{
	t_tabla_estados *registro;
	int i;

	for (i=0; i<list_size(listaTablaEstados);i++)
	{
		registro = list_get(listaTablaEstados,i);
		if (registro->job == job)
		{
			list_add(listaFiltrada,registro);
		}
	}
	return;
}

void restarJob(t_list *listaFiltrada)
{
	int i,nodoRedGlobal,cantRedLocales=0;
	t_tabla_estados *registro;

	for(i=0;i<list_size(listaFiltrada);i++)
	{
		registro = list_get(listaFiltrada,i);

		switch(registro->etapa){
		case 1:
			workers[registro->nodo].workLoad -=1;
			break;
		case 2:
			cantRedLocales ++;
			break;
		case 3:		//planificacion guardada ordenada solo entra en la ultima vuelta
			nodoRedGlobal = registro->nodo;
			break;
		}
	}

	if ((cantRedLocales % 2)==0)
	{
		cantRedLocales = cantRedLocales / 2;
	}
	else
	{
		cantRedLocales = ((cantRedLocales / 2)+1);
	}

	workers[nodoRedGlobal].workLoad-= cantRedLocales;

	return;

}


/* 						Cambiar Estado segun informe master						*/
void *cambiarEstado(char* nombreTMP, int estado)
{
	int etapaCompletada=0;
	t_tabla_estados *registro;

	registro = encontrarRegistro(nombreTMP);
	registro->estado = estado;

	etapaCompletada = verificarEtapa(registro->job,registro->etapa);

	if (etapaCompletada)
	{
		switch (registro->etapa)
		{
			case 1:
				iniciarReduccionLocales();
				break;
			case 2:
				iniciarReduccionGlobal();
				break;
		}
	}
	return (void*)0;
}

int verificarEtapa(int trabajo,int etapa)
{
	t_tabla_estados *registro;
	t_list *listaFiltrada;
	listaFiltrada = list_create();
	int i;

	for(i=0;i<list_size(listaTablaEstados);i++)
	{
		registro = list_get(listaTablaEstados,i);
		if(registro->job==trabajo && registro->etapa==etapa && registro->estado!=1)
			list_add(listaFiltrada,registro);
	}

	i = list_all_satisfy(listaFiltrada,tareaOk);

	list_destroy(listaFiltrada);

	return i;
}

bool tareaOk(void *registro)
{
	t_tabla_estados *reg;
	reg = (t_tabla_estados*) registro;

	if(reg->estado == 2)
		return 1;
	else
		return 0;

}

void iniciarReduccionLocales(void){
	return;
}

void iniciarReduccionGlobal(void){
	return;
}

/* 						all this will go in funcionesYama.c						*/


void crearListas(){
	listaBloquesRecibidos = list_create();
		listaNodosInvolucrados= list_create();
		listaPlanTransformaciones = list_create();
		listaPlanRedLocal = list_create();
		listaPlanRedGlobal = list_create();
}


void inicializarWorkers()
{
	int i;
	for (i=0;i<30;i++)
	{
		workers[i].habilitado = 1;
		workers[i].disponibilidad = 0;
		workers[i].idNodo = i;
		workers[i].puerto = 0;
		workers[i].ip = "127.0.0.1";
		workers[i].workLoad = 0;
		workers[i].cant_transformaciones = 0;
		workers[i].cant_red_globales = 0;
	}
	return;
}

void guardarEnBloqueRecibidos(t_bloqueRecv* recibido)
{
	nodosCargados(recibido->idNodo0,recibido->idNodo1);
	list_add(listaBloquesRecibidos,recibido);
}

void nodosCargados(int idNodo0,int idNodo1)
{
	int *nodoACargar1,*nodoACargar2;

	if ((!existeIdNodo(idNodo0)) && (workers[idNodo0].habilitado == 1))
	{
		nodoACargar1 = malloc(sizeof(int));
		*nodoACargar1 = idNodo0;
		list_add(listaNodosInvolucrados,nodoACargar1);
	}
	if ((!existeIdNodo(idNodo1)) && (workers[idNodo1].habilitado == 1))
	{
		nodoACargar2 = malloc(sizeof(int));
		*nodoACargar2 = idNodo1;
		list_add(listaNodosInvolucrados,nodoACargar2);
	}
}

int existeIdNodo(int val1){
	int i;
	int* val2;
	for (i=0;i<list_size(listaNodosInvolucrados);i++)
	{
		val2=(int*)list_get(listaNodosInvolucrados,i);
		if(val1 == *val2){
			return 1;
		}
	}
	return 0;
}

bool ordenarPorDisponibilidad(void* val1,void* val2){
	int nodo0,nodo1;
	nodo0 = *(int*)val1;
	nodo1 = *(int*)val2;


	if ((workers[nodo0].disponibilidad) >= (workers[nodo1].disponibilidad))
		return 1;
	else
		return 0;

}

void cargarVector(int* vectorNodos, t_list* listaNodos){
 int i=0,*idNodo;

 while (!list_is_empty(listaNodos))
 {
  idNodo = list_remove(listaNodos,0);
  vectorNodos[i] = *idNodo;
  i++;
 }

 list_destroy(listaNodos);
}
//ahi deberia funcionar

//verificar el funcionamiento del clock
int planificarTransformaciones(int cantNodos,int* nodosInvolucrados, t_bloqueRecv* bloqueRecibido,int* clock,int* clockAux){
	int asignado=0,nodosRecorridos=0;

	while((asignado==0)&&(nodosRecorridos<cantNodos))
	{
		if (nodoContieneBloque(*bloqueRecibido,nodosInvolucrados,clock))
		{
			if(workers[nodosInvolucrados[*clock]].disponibilidad>0)
			{
				actualizarPlanificacion(*bloqueRecibido,nodosInvolucrados,clock);
				actualizarWorker(nodosInvolucrados,clock);
				actualizarTablaEstados(*bloqueRecibido,nodosInvolucrados,clock);
				desplazarClock(clock,cantNodos);
				*clockAux=*clock;
				asignado=1;
			}else
			{
				incrementarDisponibilidadWorkers(cantNodos,nodosInvolucrados);
			}
		}
		else if(nodoContieneBloque(*bloqueRecibido,nodosInvolucrados,clockAux))
			{
				if(workers[nodosInvolucrados[*clockAux]].disponibilidad>0)
				{
					actualizarPlanificacion(*bloqueRecibido,nodosInvolucrados,clockAux);
					actualizarWorker(nodosInvolucrados,clockAux);
					actualizarTablaEstados(*bloqueRecibido,nodosInvolucrados,clockAux);
					*clockAux = *clock;
					asignado=1;
				}else
				{
					incrementarDisponibilidadWorkers(cantNodos,nodosInvolucrados);
				}
			}else
			{
				desplazarClock(clockAux,cantNodos);
				nodosRecorridos++;
				//falta ver cuando no esta habilitado en ningun lado
			}
	}
	if (nodosRecorridos == cantNodos)
		return 1;
	else
		return 0;
}

int nodoContieneBloque(t_bloqueRecv bloqueRecibido,int* nodosInvolucrados,int* clock)
{
	/* Verifico que el nodo en cuestion este habilitado */
	if (workers[(nodosInvolucrados[*clock])].habilitado==0)
		return 0;

	/* Verifico que el nodo en cuestion sea alguno de los que tiene el bloque*/
	if((bloqueRecibido.idNodo0)==nodosInvolucrados[*clock]||((bloqueRecibido.idNodo1)==nodosInvolucrados[*clock]))
		return 1;
	else
		return 0;
}

void actualizarPlanificacion(t_bloqueRecv bloqueRecibido,int* nodosInvolucrados,int* clock)
{
	char *nombreTMP;
	nombreTMP = string_new();
	t_transformacionMaster* bloquePlanificado;
	bloquePlanificado = malloc(sizeof(t_transformacionMaster));

	bloquePlanificado->idNodo=nodosInvolucrados[*clock];
	bloquePlanificado->bytesOcupados=bloqueRecibido.bytesOcupados;
	bloquePlanificado->nroBloqueNodo=cargarNroBloque(bloqueRecibido,nodosInvolucrados,clock);
	bloquePlanificado->puerto=workers[nodosInvolucrados[*clock]].puerto;
	bloquePlanificado->largoIp = strlen(workers[nodosInvolucrados[*clock]].ip)+1;
	bloquePlanificado->ip=malloc(bloquePlanificado->largoIp);
	strcpy(bloquePlanificado->ip,workers[nodosInvolucrados[*clock]].ip);

	nombreTMP=string_duplicate(generarNombreArchivoTemporal(job,bloquePlanificado->idNodo,bloquePlanificado->nroBloqueNodo));
	bloquePlanificado->largoArchivo = strlen(nombreTMP)+1;
	bloquePlanificado->archivoTransformacion = malloc(bloquePlanificado->largoArchivo);
	strcpy(bloquePlanificado->archivoTransformacion,nombreTMP);

	list_add(listaPlanTransformaciones,bloquePlanificado);

	return;
}

int cargarNroBloque(t_bloqueRecv bloque,int* nodosInvolucrados,int* clock)
{
	if (bloque.idNodo0==nodosInvolucrados[*clock])
		return bloque.nroBloqueNodo0;
	else
		return bloque.nroBloqueNodo1;
}

void desplazarClock(int* clock,int cantidad){

	if(*clock < cantidad-1)
		*clock=*clock+1;
	else
		*clock = 0;
	return;
}

void actualizarWorker(int* nodosInvolucrados,int* clock)
{
	int idNodo = nodosInvolucrados[*clock];
	workers[idNodo].cant_transformaciones++;
	workers[idNodo].workLoad++;
	workers[idNodo].disponibilidad--;
}
void restaurarWorkload()
{
	int i=0;
	t_transformacionMaster *registro;

	for(i=0;list_size(listaPlanTransformaciones);i++)
	{
		registro = list_get(listaPlanTransformaciones,i);
		workers[registro->idNodo].cant_transformaciones--;
		workers[registro->idNodo].workLoad--;
		workers[registro->idNodo].disponibilidad++;
	}

}
void actualizarTablaEstados(t_bloqueRecv bloqueRecibido,int* nodosInvolucrados,int* clock){
	t_tabla_estados* registroEstado;
	registroEstado = malloc(sizeof(t_tabla_estados));
	registroEstado->job = job;
	registroEstado->master = idMaster;
	registroEstado->nodo = nodosInvolucrados[*clock];
	registroEstado->bloque = cargarNroBloque(bloqueRecibido,nodosInvolucrados,clock);
	registroEstado->etapa = 1; //ver cual va a ser la macro de etapa
	registroEstado->estado = 1; //ver cual va a ser la macro de estado
	char* temp = string_new();
	temp = string_duplicate(generarNombreArchivoTemporal(job, registroEstado->nodo, registroEstado->bloque));

	registroEstado->archivoTemp = malloc(strlen(temp)+1);
	strcpy(registroEstado->archivoTemp,temp);
	list_add(listaTablaEstados, registroEstado);
}

char* generarNombreArchivoTemporal(int job, int nodo, int bloque){
	char* nombre;
	char* jobString, *nodoString, *bloqueString;
	nombre = string_new();
	jobString = string_new();
	nodoString = string_new();
	bloqueString = string_new();

	jobString = string_itoa(job);
	nodoString = string_itoa(nodo);
	bloqueString = string_itoa(bloque);

	string_append(&nombre, "/tmp/");
	string_append_with_format(&nombre, "j%s", jobString);

	if(bloque == 9000){ //si es reduccion global
		string_append(&nombre, "rg");
	}else{
		string_append_with_format(&nombre, "n%s", nodoString);
		if(bloque == 8000){
		string_append(&nombre,"rl");
		}else{ //si es reduccion local
			string_append_with_format(&nombre, "b%s", bloqueString);
		}
	}
	return nombre;
}

void incrementarDisponibilidadWorkers(int cantNodos,int* nodosInvolucrados){

	int i;

	for (i=0;i<cantNodos;i++)
	{
		workers[nodosInvolucrados[i]].disponibilidad+=disponibilidadBase;
	}
	return;
}


/*								Reduccion Local								*/

void planificacionReduccionLocal()
{
	int cantTransformaciones = list_size(listaPlanTransformaciones);
	int nodoActual, i=0;
	t_transformacionMaster *transformacion;
	t_reduccionLocalMaster *redLocal;
	char *nombreTMP;

	//transformacion = malloc(sizeof(t_transformacionMaster));
	redLocal = malloc(sizeof(t_reduccionLocalMaster));
	nombreTMP = string_new();
	transformacion = list_get(listaPlanTransformaciones,i);

	while(i<cantTransformaciones)
	{
		nodoActual = transformacion->idNodo;

		nombreTMP = string_duplicate(generarNombreArchivoTemporal(job, transformacion->idNodo, 8000));

		while ((i<cantTransformaciones)&&(nodoActual==transformacion->idNodo))
		{
			cargarInfoReduccionLocal(transformacion,redLocal,nombreTMP);
			if(existeRedLocal(redLocal,listaPlanRedLocal))
				cargarReduccionLocalTablaEstado(nombreTMP,redLocal->idNodo);
			list_add(listaPlanRedLocal,redLocal);
			i++;
			transformacion=list_get(listaPlanTransformaciones,i);
			redLocal = malloc(sizeof(t_reduccionLocalMaster));
		}
	}

	free(nombreTMP);

	return;
}

void cargarInfoReduccionLocal(t_transformacionMaster *transformacion, t_reduccionLocalMaster *reduccion, char *nombreTMP)
{
	reduccion->idNodo = transformacion->idNodo;
	reduccion->puerto = transformacion->puerto;

	reduccion->largoArchivoRedLocal = strlen(nombreTMP)+1;
	reduccion->archivoRedLocal = malloc(reduccion->largoArchivoRedLocal);
	strcpy(reduccion->archivoRedLocal,nombreTMP);

	reduccion->largoIp = strlen(transformacion->ip)+1;
	reduccion->ip = malloc(reduccion->largoIp);
	strcpy(reduccion->ip,transformacion->ip);

	reduccion->largoArchivoTransformacion = strlen(transformacion->archivoTransformacion)+1;
	reduccion->archivoTransformacion = malloc(reduccion->largoArchivoTransformacion);
	strcpy(reduccion->archivoTransformacion,transformacion->archivoTransformacion);

	return;
}

void cargarReduccionLocalTablaEstado(char* nombreTMP,int idNodo)
{
	t_tabla_estados *registro;
	registro = malloc(sizeof(t_tabla_estados));
	registro->archivoTemp = malloc(strlen(nombreTMP)+1);
	strcpy(registro->archivoTemp,nombreTMP);
	registro->estado=1;// 1 en progreso
	registro->etapa=2;// 2 para reduccion local
	registro->job = job;
	registro->master = idMaster;
	registro->nodo = idNodo;
	registro->bloque=8000;
	list_add(listaTablaEstados,registro);
	return;

}

/*  							Reduccion global 							*/
void planificacionReduccionGlobal(int cantNodos,int *nodosInvolucrados)
{
	int nodoReduccionGlobal,i;
	t_list *listaAux;
	listaAux = list_create();

	seleccionarTransformacionLocales(listaAux);

	t_reduccionGlobalMaster *infoReduccionGlobal;

	nodoReduccionGlobal = seleccionarNodoMenorCarga(nodosInvolucrados,cantNodos);
	workers[nodoReduccionGlobal].cant_red_globales++;

	for (i=0;i<list_size(listaAux);i++)
	{
		infoReduccionGlobal= malloc(sizeof(t_reduccionGlobalMaster));
		cargarInfoReduccionGlobal(i,nodoReduccionGlobal,infoReduccionGlobal,listaAux);
		if(infoReduccionGlobal->idNodo == nodoReduccionGlobal)
		{
			cargarReduccionGlobalTablaEstados(nodoReduccionGlobal,infoReduccionGlobal);
		}

		list_add(listaPlanRedGlobal,infoReduccionGlobal);
	}

	list_destroy(listaAux);
	return;
}

/* Genero una lista con solo las diferentes reducciones locales, elimino las repetidas */
void seleccionarTransformacionLocales(t_list *lista)
{
	int i;
	t_reduccionLocalMaster *registro;
	//registro = malloc(sizeof(t_reduccionLocalMaster));

	for (i=0;i<list_size(listaPlanRedLocal);i++)
	{
		registro = list_get(listaPlanRedLocal,i);

		if (existeRedLocal(registro,lista))
		{
			list_add(lista,registro);
			//registro = malloc(sizeof(t_reduccionLocalMaster));
		}
	}
}

/* verifico si la reduccion local ya se encuntra en la lista */
bool existeRedLocal(t_reduccionLocalMaster *reduccion, t_list* lista)
{
	int i=0;

	t_reduccionLocalMaster *elementoLista;

	//elementoLista = malloc(sizeof(t_reduccionLocalMaster));

	if (list_size(lista)>0)
	{
		for (i=0; i<list_size(lista);i++)
		{
			elementoLista = list_get(lista,i);
			if(sonIguales(reduccion->archivoRedLocal,elementoLista->archivoRedLocal))
			{
				return 0;
			}
		}
	}
	else
	{
		return 1;
	}

	return 1;
}

int seleccionarNodoMenorCarga(int* nodosInvolucrados,int cantNodos)
{
	int i,nodoMenorCarga;
	t_list *listAux;
	listAux = list_create();

	for (i=0;i<cantNodos;i++)
	{
		if (tieneReduccionesLocales(nodosInvolucrados[i]))
		{
			list_add(listAux,(int*)nodosInvolucrados[i]);
		}
	}

	list_sort(listAux,ordenarPorWorkload);

	nodoMenorCarga = (int) list_get(listAux,0);

	return nodoMenorCarga;

//crear lista con nodos que tengan reducciones locales
// sort list by worload
//select first element of the list


	/*int i, nodoMenorCarga=nodosInvolucrados[0];

	for (i=0;i<cantNodos;i++)
	{
		if(nodoIntervinoEnJob(nodosInvolucrados[i]))
		{
			if(workers[nodoMenorCarga].workLoad>workers[nodosInvolucrados[i]].workLoad)
			{
				nodoMenorCarga = nodosInvolucrados[i];
			}
		}
	}
	return nodoMenorCarga;*/
}

bool ordenarPorWorkload(void* val1,void* val2){
	int nodo0,nodo1;
	nodo0 = *(int*)val1;
	nodo1 = *(int*)val2;


	if ((workers[nodo0].workLoad) <= (workers[nodo1].workLoad))
		return 1;
	else
		return 0;

}

int tieneReduccionesLocales(int nodo)
{
	t_reduccionLocalMaster *registro;
	int i;

	for (i=0;i<list_size(listaPlanRedLocal);i++)
	{
		registro = (t_reduccionLocalMaster*) list_get(listaPlanRedLocal,i);
		if (registro->idNodo == nodo)
			return 1;
	}
	return 0;
}

void cargarInfoReduccionGlobal(int posicion,int nodoReduccionGlobal,t_reduccionGlobalMaster* infoReduccionGlobal,t_list *lista)
{

	char *nombreTMP,*reduccionLocal;
	t_reduccionLocalMaster *registro;

	/* Obtener el nombre del archivo de reduccion local */
	//registro = malloc(sizeof(t_reduccionLocalMaster));
	registro=(t_reduccionLocalMaster*)list_get(lista,posicion);

	infoReduccionGlobal->largoArchivoRedLocal = strlen(registro->archivoRedLocal)+1;
	reduccionLocal = malloc(infoReduccionGlobal->largoArchivoRedLocal);
	infoReduccionGlobal->archivoRedLocal = malloc(infoReduccionGlobal->largoArchivoRedLocal);

	strcpy(reduccionLocal,registro->archivoRedLocal);
	strcpy(infoReduccionGlobal->archivoRedLocal,registro->archivoRedLocal);

	/* Cargar el resto de los campos del registro y agregarlo a la lista */
	infoReduccionGlobal->idNodo = registro->idNodo;
	infoReduccionGlobal->puerto=workers[registro->idNodo].puerto;

	infoReduccionGlobal->largoIp = strlen(workers[registro->idNodo].ip)+1;
	infoReduccionGlobal->ip=malloc(infoReduccionGlobal->largoIp);
	strcpy(infoReduccionGlobal->ip,workers[registro->idNodo].ip);
	nombreTMP=generarNombreArchivoTemporal(job,nodoReduccionGlobal,9000);

	/* Verifica si es el nodo encargado y completa acordemente */
	if (registro->idNodo == nodoReduccionGlobal)
	{
		infoReduccionGlobal->encargado = 1;
		infoReduccionGlobal->largoArchivoRedGlobal=strlen(nombreTMP)+1;
		infoReduccionGlobal->archivoRedGlobal=malloc(infoReduccionGlobal->largoArchivoRedGlobal);
		strcpy(infoReduccionGlobal->archivoRedGlobal,nombreTMP);
	}
	else
	{
		//si no es el encargado no le interesa el nombre del archivoRedGlobal
		infoReduccionGlobal->archivoRedGlobal = malloc(sizeof(char));
		infoReduccionGlobal->largoArchivoRedGlobal=0;
		strcpy(infoReduccionGlobal->archivoRedGlobal,"");
		infoReduccionGlobal->encargado = 0;
	}
	/* Release the memory */
	free(nombreTMP);
	free(reduccionLocal);
	return;
}

void cargarReduccionGlobalTablaEstados(int nodoReduccion,t_reduccionGlobalMaster *infoRedGlobal)
{
	t_tabla_estados *registro;
	registro = malloc(sizeof(t_tabla_estados));

	registro->archivoTemp = malloc(strlen(infoRedGlobal->archivoRedGlobal)+1);
	strcpy(registro->archivoTemp,infoRedGlobal->archivoRedGlobal);
	registro->bloque = 9000;
	registro->estado = 1;
	registro->etapa = 3;
	registro->job = job;
	registro->master = idMaster;
	registro->nodo = infoRedGlobal->idNodo;

	list_add(listaTablaEstados,registro);

	return;
}

void actualizarWorkload(int cantNodosInvolucrados,int* nodosInvolucrados)
{
	int i,prevWorkLoad,difCarga,maxWorkload=workers[0].workLoad;

	/* Buscar maxima carga entre todos los workers */
	for (i=1;i<30;i++)
	{
		if(maxWorkload<workers[i].workLoad)
			maxWorkload=workers[i].workLoad;
	}

	/* Se actualiza segun algoritmo de planificacion */
	if (algoritmo==1)
	{
		for (i=0;i<cantNodosInvolucrados;i++)
		{
			prevWorkLoad=workers[nodosInvolucrados[i]].workLoad;
			difCarga= maxWorkload-prevWorkLoad;
			workers[nodosInvolucrados[i]].disponibilidad=disponibilidadBase+difCarga;
		}
	}
	else
	{
		for (i=0;i<cantNodosInvolucrados;i++)
		{
			workers[i].disponibilidad = disponibilidadBase;
		}
	}

	return;
}


/*								Destruir listas								 */
void destruir_listas()
{
	list_destroy_and_destroy_elements(listaPlanTransformaciones,freeTransformaciones);
	list_destroy_and_destroy_elements(listaPlanRedLocal,freeRedLocales);
	list_destroy_and_destroy_elements(listaPlanRedGlobal,freeRedGlobal);
	return;
}

void freeTransformaciones(void *registro)
{
	t_transformacionMaster *reg;

	reg = (t_transformacionMaster*) registro;

	free(reg->archivoTransformacion);
	free(reg->ip);
	free(reg);

}

void freeRedLocales(void *registro)
{
	t_reduccionLocalMaster *reg;

	reg = (t_reduccionLocalMaster*) registro;

	free(reg->archivoRedLocal);
	free(reg->archivoTransformacion);
	free(reg->ip);
	free(reg);
}

void freeRedGlobal(void *registro)
{
	t_reduccionGlobalMaster *reg;

	reg = (t_reduccionGlobalMaster*) registro;
	if(reg->encargado==1)
		free(reg->archivoRedGlobal);
	free(reg->archivoRedLocal);
	free(reg->ip);

	free(reg);
}

void actualizarConfig()
{
	char* path = "YAMAconfig.cfg";
	char cwd[1024];
	char *pathArchConfig = string_from_format("%s/%s", getcwd(cwd, sizeof(cwd)),
			path);
	t_config *config = config_create(pathArchConfig);

	int estadoGuardado = 0;

	if (!config) {
		perror("[ERROR]: No se pudo cargar el archivo de configuracion.");
		exit(EXIT_FAILURE);
	}

	if (config_has_property(config, "JOB"))
		{
			job = config_get_int_value(config, "JOB");
		}

	if (config_has_property(config,"JOB"))
		{
			config_set_value(config,"JOB",string_itoa(job+1));
			estadoGuardado = config_save(config);
		}

	if (estadoGuardado==(-1))
	{
		perror("[ERROR]: Fallo al guardar YAMAconfig.cfg");
	}

	if (config_has_property(config,"ALGORITMO_BALANCEO"))
		algoritmo = config_get_int_value(config,"ALGORITMO_BALANCEO");

	return;
}

void *rePrePlanificacion(char *ArchivoTrabajo,char *archivoTMP,t_job* jobRePlanificado)
{
	t_tabla_estados *registro;
	pthread_t planificacion;


	registro = encontrarRegistro(archivoTMP);

	/* Descargo la carga del trabajo previamente planificado */
	descargarWorkload(archivoTMP);

	/* Deshabilito nodo caido */
	workers[registro->nodo].habilitado=0;

	/* ejecuto hilo de planificacion */
	pthread_create(&planificacion,NULL,preplanificarJob(jobRePlanificado),NULL);

	pthread_join(planificacion,NULL);

	return (void*)1;
}

/*                     		Enviar la planificacion a master                  */


void enviarMensajeFalloOperacion(t_job* jobMaster)
{
	void *bufferMensaje;
	int desplazamiento=0;
	t_header header;

	header.id=101;
	header.tamanioPayload=0;

	bufferMensaje = malloc(sizeof(t_header));

	memcpy(bufferMensaje, &header.id,sizeof(header.id));
	desplazamiento += sizeof(header.id);

	memcpy(bufferMensaje+desplazamiento, &header.tamanioPayload,sizeof(header.tamanioPayload));
	desplazamiento += sizeof(header.tamanioPayload);

	enviarPorSocket(jobMaster->socketMaster,bufferMensaje, 8);

	return;
}

void enviarPlanificacionAMaster(t_job* jobMaster){

	uint32_t cantTransformaciones = list_size(listaPlanTransformaciones);
	uint32_t cantRedLocal = list_size(listaPlanRedLocal);
	uint32_t cantRedGlobal = list_size(listaPlanRedGlobal);
	int desplazamiento = 0;
	int largoTransformaciones, largoRedLocales , largoRedGlobales, tamanioTotalBuffer;
	void* bufferTransformaciones;
	void* bufferRedLocal;
	void* bufferRedGlobal;
	void* bufferMensaje;
	printf("transformaciones: %d,  red locales: %d: red globales: %d\n", cantTransformaciones, cantRedLocal, cantRedGlobal);

	bufferTransformaciones = serializarTransformaciones(cantTransformaciones, &largoTransformaciones, listaPlanTransformaciones);

	bufferRedLocal = serializarRedLocales(cantRedLocal, &largoRedLocales, listaPlanRedLocal);

	bufferRedGlobal = serializarRedGlobales(cantRedGlobal, &largoRedGlobales, listaPlanRedGlobal);

	printf("largo trans: %d, redLoc %d, redGlo %d\n",largoTransformaciones,largoRedLocales, largoRedGlobales);

	tamanioTotalBuffer = largoTransformaciones + largoRedLocales + largoRedGlobales +
			sizeof(cantTransformaciones) + sizeof(cantRedLocal) + sizeof(cantRedGlobal);

	t_header header;
	header.id = 5;
	header.tamanioPayload = tamanioTotalBuffer;

	bufferMensaje = malloc(tamanioTotalBuffer+sizeof(t_header));

	memcpy(bufferMensaje, &header.id,sizeof(header.id));
	desplazamiento += sizeof(header.id);

	memcpy(bufferMensaje+desplazamiento, &header.tamanioPayload,sizeof(header.tamanioPayload));
	desplazamiento += sizeof(header.tamanioPayload);


	memcpy(bufferMensaje+desplazamiento, &cantTransformaciones,sizeof(cantTransformaciones));
	desplazamiento += sizeof(cantTransformaciones);

	memcpy(bufferMensaje+desplazamiento, &cantRedLocal,sizeof(cantRedLocal));
	desplazamiento += sizeof(cantRedLocal);

	memcpy(bufferMensaje+desplazamiento, &cantRedGlobal, sizeof(cantRedGlobal));
	desplazamiento += sizeof(cantRedGlobal);

	memcpy(bufferMensaje+desplazamiento,bufferTransformaciones,largoTransformaciones);
	desplazamiento += largoTransformaciones;

	memcpy(bufferMensaje+desplazamiento,bufferRedLocal,largoRedLocales);
	desplazamiento += largoRedLocales;

	memcpy(bufferMensaje+desplazamiento, bufferRedGlobal, largoRedGlobales);
	desplazamiento += largoRedGlobales;

	enviarPorSocket(jobMaster->socketMaster,bufferMensaje, tamanioTotalBuffer+8);

	printf("envia planificacion a master\n");

	free(bufferTransformaciones);
	free(bufferRedLocal);
	free(bufferRedGlobal);


	free(bufferMensaje);
}

void* serializarTransformaciones(int cantTransformaciones, int* largoMensaje, t_list* lista){
	void* buffer;
	int i, desplazamiento = 0;
	t_transformacionMaster* transformacion = malloc(sizeof(t_transformacionMaster));
	buffer = malloc(sizeof(t_transformacionMaster));

	for(i=0;i<cantTransformaciones ;i++){
			transformacion = (t_transformacionMaster*) list_get(lista, i);
			buffer = realloc(buffer, sizeof(t_transformacionMaster) +
					transformacion->largoArchivo + transformacion->largoIp + desplazamiento);

//			printf("%s, %s\n", transformacion->ip, transformacion->archivoTransformacion);
	//		printf("%d, %d\n", transformacion->largoIp, transformacion->largoArchivo);

			memcpy(buffer+desplazamiento, &transformacion->idNodo, sizeof(transformacion->idNodo));
			desplazamiento+=sizeof(transformacion->idNodo);
			memcpy(buffer+desplazamiento, &transformacion->nroBloqueNodo, sizeof(transformacion->nroBloqueNodo));
			desplazamiento+=sizeof(transformacion->nroBloqueNodo);
			memcpy(buffer+desplazamiento, &transformacion->bytesOcupados, sizeof(transformacion->bytesOcupados));
			desplazamiento+=sizeof(transformacion->bytesOcupados);
			memcpy(buffer+desplazamiento, &transformacion->puerto, sizeof(transformacion->puerto));
			desplazamiento+=sizeof(transformacion->puerto);
			memcpy(buffer+desplazamiento, &transformacion->largoIp, sizeof(transformacion->largoIp));
			desplazamiento+=sizeof(transformacion->largoIp);
			memcpy(buffer+desplazamiento, transformacion->ip, transformacion->largoIp);
			desplazamiento+=transformacion->largoIp;
			memcpy(buffer+desplazamiento, &transformacion->largoArchivo, sizeof(transformacion->largoArchivo));
			desplazamiento+=sizeof(transformacion->largoArchivo);
			memcpy(buffer+desplazamiento, transformacion->archivoTransformacion, transformacion->largoArchivo);
			desplazamiento+=transformacion->largoArchivo;

		}
	*largoMensaje = desplazamiento;
	free(transformacion);
	return buffer;
}

void* serializarRedLocales(int cantReducciones, int* largoMensaje, t_list* lista){
	void* buffer;
	int i, desplazamiento = 0;
	t_reduccionLocalMaster* redLocal = malloc(sizeof(t_reduccionLocalMaster));

	buffer = malloc(sizeof(t_reduccionLocalMaster));

	for(i=0;i<cantReducciones ;i++){
			redLocal = (t_reduccionLocalMaster*) list_get(lista, i);
			buffer = realloc(buffer, sizeof(t_reduccionLocalMaster) +
					redLocal->largoArchivoTransformacion + redLocal->largoArchivoRedLocal +  redLocal->largoIp + desplazamiento);

		//	printf("%s, %s, %s\n", redLocal->ip, redLocal->archivoTransformacion, redLocal->archivoRedLocal);
			//printf("%d, %d, %d\n", redLocal->largoIp, redLocal->largoArchivoTransformacion, redLocal->largoArchivoRedLocal);

			memcpy(buffer+desplazamiento, &redLocal->idNodo, sizeof(redLocal->idNodo));
			desplazamiento+=sizeof(redLocal->idNodo);
			memcpy(buffer+desplazamiento, &redLocal->puerto, sizeof(redLocal->puerto));
			desplazamiento+=sizeof(redLocal->puerto);
			memcpy(buffer+desplazamiento, &redLocal->largoIp, sizeof(redLocal->largoIp));
			desplazamiento+=sizeof(redLocal->largoIp);
			memcpy(buffer+desplazamiento, redLocal->ip, redLocal->largoIp);
			desplazamiento+=redLocal->largoIp;
			memcpy(buffer+desplazamiento, &redLocal->largoArchivoTransformacion, sizeof(redLocal->largoArchivoTransformacion));
			desplazamiento+=sizeof(redLocal->largoArchivoTransformacion);
			memcpy(buffer+desplazamiento, redLocal->archivoTransformacion, redLocal->largoArchivoTransformacion);
			desplazamiento+=redLocal->largoArchivoTransformacion;
			memcpy(buffer+desplazamiento, &redLocal->largoArchivoRedLocal, sizeof(redLocal->largoArchivoRedLocal));
			desplazamiento+=sizeof(redLocal->largoArchivoRedLocal);
			memcpy(buffer+desplazamiento, redLocal->archivoRedLocal, redLocal->largoArchivoRedLocal);
			desplazamiento+=redLocal->largoArchivoRedLocal;
		}
	*largoMensaje = desplazamiento;
	free(redLocal);
	return buffer;
}

void* serializarRedGlobales(int cantReducciones, int* largoMensaje, t_list* lista){
	void* buffer;
	int i, desplazamiento = 0;
	t_reduccionGlobalMaster* redGlobal = malloc(sizeof(t_reduccionGlobalMaster));

	buffer = malloc(sizeof(t_reduccionGlobalMaster));

	for(i=0;i<cantReducciones ;i++){
			redGlobal = (t_reduccionGlobalMaster*) list_get(lista, i);
			buffer = realloc(buffer, sizeof(t_reduccionGlobalMaster) +
					redGlobal->largoArchivoRedGlobal + redGlobal->largoArchivoRedLocal +  redGlobal->largoIp + desplazamiento);

		//	printf("%s, %s, %s\n", redGlobal->ip, redGlobal->archivoRedLocal, redGlobal->archivoRedGlobal);
			//printf("%d, %d, %d\n", redGlobal->largoIp, redGlobal->largoArchivoRedLocal, redGlobal->largoArchivoRedGlobal);

			memcpy(buffer+desplazamiento, &redGlobal->idNodo, sizeof(redGlobal->idNodo));
			desplazamiento+=sizeof(redGlobal->idNodo);
			memcpy(buffer+desplazamiento, &redGlobal->encargado, sizeof(redGlobal->encargado));
			desplazamiento+=sizeof(redGlobal->encargado);
			memcpy(buffer+desplazamiento, &redGlobal->puerto, sizeof(redGlobal->puerto));
			desplazamiento+=sizeof(redGlobal->puerto);
			memcpy(buffer+desplazamiento, &redGlobal->largoIp, sizeof(redGlobal->largoIp));
			desplazamiento+=sizeof(redGlobal->largoIp);
			memcpy(buffer+desplazamiento, redGlobal->ip, redGlobal->largoIp);
			desplazamiento+=redGlobal->largoIp;
			memcpy(buffer+desplazamiento, &redGlobal->largoArchivoRedLocal, sizeof(redGlobal->largoArchivoRedLocal));
			desplazamiento+=sizeof(redGlobal->largoArchivoRedLocal);
			memcpy(buffer+desplazamiento, redGlobal->archivoRedLocal, redGlobal->largoArchivoRedLocal);
			desplazamiento+=redGlobal->largoArchivoRedLocal;
			memcpy(buffer+desplazamiento, &redGlobal->largoArchivoRedGlobal, sizeof(redGlobal->largoArchivoRedGlobal));
			desplazamiento+=sizeof(redGlobal->largoArchivoRedGlobal);
			memcpy(buffer+desplazamiento, redGlobal->archivoRedGlobal, redGlobal->largoArchivoRedGlobal);
			desplazamiento+=redGlobal->largoArchivoRedGlobal;

		}
	*largoMensaje = desplazamiento;
	free(redGlobal);
	return buffer;
}
/*          Envio peticion a FS para iniciar operacion de planificacion      */




void serializarPeticionInfoArchivo(void *paquete,char *rutaArchivo,char *rutaGuardadoFinal){
int desplazamiento=0;
uint32_t *largoRuta=malloc(sizeof(uint32_t));
*largoRuta=strlen(rutaArchivo)+1;
memcpy(paquete,largoRuta,sizeof(uint32_t));
desplazamiento+=sizeof(uint32_t);

 memcpy(paquete+desplazamiento,rutaArchivo,*largoRuta);
 desplazamiento+=*largoRuta;

 *largoRuta=strlen(rutaGuardadoFinal)+1;
 memcpy(paquete+desplazamiento,largoRuta,sizeof(uint32_t));
 desplazamiento+=sizeof(uint32_t);

 memcpy(paquete+desplazamiento,rutaGuardadoFinal,*largoRuta);
 desplazamiento+=*largoRuta;

 free(largoRuta);
}

