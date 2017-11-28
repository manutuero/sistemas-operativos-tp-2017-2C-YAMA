#include "funcionesPlanificacion.h"

t_worker_Disponibles workers[30];

int algoritmo = 1;//idem anterior, 0 para clock y 1 para wclock
//uint32_t job = 0;
int idMaster = 1;
uint32_t disponibilidadBase;

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

	//t_bloqueRecv* bloqueRecibido;
	int i,cantNodosInvolucrados,*clock,*clockAux,transformacionesOk=0;


	crearListas();

	/* ENFS envio de nombres de archivos a FS */

	envioPedidoArchivoAFS(jobMaster->pedidoTransformacion);

	/* Reservar memoria para los clocks */

	clock = malloc(sizeof(uint32_t));
	clockAux = malloc(sizeof(uint32_t));

	/* RBFS    Recibir de FS la composicion completa del archivo          */

	recibirComposicionArchivo();


	/*for (i=0;i<header.tamanioPayload;i++)
	{
		//Cambio bloqueRecibido por buffer en el malloc
		buffer = malloc(sizeof(t_bloqueRecv));//Se liberara cuando se destruya la lista
		bloqueRecibido = malloc(sizeof(t_bloqueRecv));//Se liberara cuando se destruya la lista
		recibirPorSocket(socketFS, buffer, sizeof(t_bloqueRecv));
		bloqueRecibido=(t_bloqueRecv*)buffer;

		//prueba local (sin sockets)
		//bloqueRecibido=&bloques[i];
		guardarEnBloqueRecibidos(bloqueRecibido);

	}*/

	/*ACFG          ACTUALIZAR CONFIG POR  INCREMENTO DE JOB  		  */
	actualizarConfig();

	/*PNIN   			  Preparar Nodos Involucrados					   */

	t_bloqueRecv* bloqueRecibido;
	for(i=0;i<list_size(listaBloquesRecibidos);i++){
		bloqueRecibido = (t_bloqueRecv*)list_get(listaBloquesRecibidos,i);
			printf("bloque  %d  nodo0: %d    nodo1: %d\n",bloqueRecibido->nroBloqueArch, bloqueRecibido->idNodo0, bloqueRecibido->idNodo1);
		}

	//Preguntar a tocayo!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	/*printf("nodos involucrados: \n");
		for(i=0;i<list_size(listaNodosInvolucrados);i++){
			nodo = *(int*)list_get(listaNodosInvolucrados,i);
			printf("nodo %d\n",*(int*)list_get(listaNodosInvolucrados,i));
		}*/

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

	/* IPLA 			     	Inicia la planificacion 					     */

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

	/*LMPZ         liberar listas para la siguiente planificacion			 */
	destruir_listas();

	/* Liberar variables dinamicas */
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
int cambiarEstado(char* nombreTMP, int estado)
{
	int etapaCompletada=0;
	t_tabla_estados *registro;

	registro = encontrarRegistro(nombreTMP);
	registro->estado = estado;

	etapaCompletada = verificarEtapa(registro->job,registro->etapa);

	return etapaCompletada;
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
		if(registro->job==trabajo && registro->etapa==etapa && registro->estado!=ERROR_TAREA)
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

	if(reg->estado == COMPLETADO)
		return 1;
	else
		return 0;

}


/* 						all this will go in funcionesYama.c						*/


void crearListas()
{
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

void nodosCargados(uint32_t idNodo0,uint32_t idNodo1)
{
	//int *nodoACargar1,*nodoACargar2;

	if ((!existeIdNodo(idNodo0)) && (workers[idNodo0].habilitado == 1))
	{
		uint32_t* nodoACargar1 = malloc(sizeof(uint32_t));
		*nodoACargar1 = idNodo0;
		//uint32_t nodoACargar1 = idNodo0;
		list_add(listaNodosInvolucrados, nodoACargar1);
	}
	if ((!existeIdNodo(idNodo1)) && (workers[idNodo1].habilitado == 1))
	{
		uint32_t* nodoACargar2 = malloc(sizeof(uint32_t));
		*nodoACargar2 = idNodo1;
		//uint32_t nodoACargar2 = idNodo1;
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
	//nombreTMP = string_new();
	t_transformacionMaster* bloquePlanificado;
	bloquePlanificado = malloc(sizeof(t_transformacionMaster));

	bloquePlanificado->idNodo=nodosInvolucrados[*clock];
	bloquePlanificado->bytesOcupados=bloqueRecibido.bytesOcupados;
	bloquePlanificado->nroBloqueNodo=cargarNroBloque(bloqueRecibido,nodosInvolucrados,clock);
	bloquePlanificado->puerto=workers[nodosInvolucrados[*clock]].puerto;
	bloquePlanificado->largoIp = strlen(workers[nodosInvolucrados[*clock]].ip)+1;
	bloquePlanificado->ip=malloc(bloquePlanificado->largoIp);
	strcpy(bloquePlanificado->ip,workers[nodosInvolucrados[*clock]].ip);

	nombreTMP=generarNombreArchivoTemporal(job,bloquePlanificado->idNodo,bloquePlanificado->nroBloqueNodo);
	bloquePlanificado->largoArchivo = strlen(nombreTMP)+1;
	bloquePlanificado->archivoTransformacion = malloc(bloquePlanificado->largoArchivo);
	strcpy(bloquePlanificado->archivoTransformacion,nombreTMP);

	list_add(listaPlanTransformaciones,bloquePlanificado);

	free(nombreTMP);
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
	registroEstado->nroBloqueArch=bloqueRecibido.nroBloqueArch;
	registroEstado->nodo = nodosInvolucrados[*clock];
	registroEstado->bloque = cargarNroBloque(bloqueRecibido,nodosInvolucrados,clock);
	registroEstado->etapa = TRANSFORMACION; //ver cual va a ser la macro de etapa
	registroEstado->estado = PROCESANDO; //ver cual va a ser la macro de estado
	char* temp;// = string_new();
	temp = generarNombreArchivoTemporal(job, registroEstado->nodo, registroEstado->bloque);
	registroEstado->archivoTemp = malloc(strlen(temp)+1);
	strcpy(registroEstado->archivoTemp,temp);
	list_add(listaTablaEstados, registroEstado);
	free(temp);
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
	free(jobString);
	free(nodoString);
	free(bloqueString);
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


	redLocal = malloc(sizeof(t_reduccionLocalMaster));
	transformacion = list_get(listaPlanTransformaciones,i);

	while(i<cantTransformaciones)
	{
		nodoActual = transformacion->idNodo;

		nombreTMP = generarNombreArchivoTemporal(job, transformacion->idNodo, 8000);

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
		free(nombreTMP);
	}

	//free(nombreTMP);

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
	registro->estado=PROCESANDO;// 1 en progreso
	registro->etapa=REDUCCION_LOCAL;// 2 para reduccion local
	registro->job = job;
	registro->master = idMaster;
	registro->nroBloqueArch=8000;
	registro->nodo = idNodo;
	registro->bloque=8000;
	list_add(listaTablaEstados,registro);
	return;

}

/*  							Reduccion global 							*/

void planificacionReduccionGlobal(int cantNodos,int *nodosInvolucrados)
{
	int nodoReduccionGlobal,i,cantRedLocales;
	t_list *listaAux;
	listaAux = list_create();

	seleccionarTransformacionLocales(listaAux);

	t_reduccionGlobalMaster *infoReduccionGlobal;

	cantRedLocales=list_size(listaPlanRedLocal);

	nodoReduccionGlobal = seleccionarNodoMenorCarga(nodosInvolucrados,cantNodos);
	workers[nodoReduccionGlobal].cant_red_globales++;

	if((cantRedLocales/2)==0)
	{
		workers[nodoReduccionGlobal].workLoad+=(cantRedLocales/2);
	}else
	{
		workers[nodoReduccionGlobal].workLoad+=(cantRedLocales/2)+1;
	}

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

	for (i=0;i<list_size(listaPlanRedLocal);i++)
	{
		registro = list_get(listaPlanRedLocal,i);

		if (existeRedLocal(registro,lista))
		{
			list_add(lista,registro);
		}
	}
}

/* verifico si la reduccion local ya se encuntra en la lista */
bool existeRedLocal(t_reduccionLocalMaster *reduccion, t_list* lista)
{
	int i=0;

	t_reduccionLocalMaster *elementoLista;

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
			list_add(listAux,&nodosInvolucrados[i]);
		}
	}

	list_sort(listAux,ordenarPorWorkload);

	nodoMenorCarga = *((int*) list_get(listAux,0));

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
	registro->nroBloqueArch=9000;
	registro->bloque = 9000;
	registro->estado = PROCESANDO;
	registro->etapa = REDUCCION_GLOBAL;
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
	char* clave, *sJob;
	char cwd[1024];
	char *pathArchConfig = string_new();
	char* ruta = string_from_format("%s/%s", getcwd(cwd, sizeof(cwd)),
			path);
	string_append(&pathArchConfig,ruta);
	//t_config *config = config_create(pathArchConfig);
	FILE* fpConfig = fopen(pathArchConfig,"r+");
	int estadoGuardado = 0;

	if (!fpConfig) {
		perror("[ERROR]: No se pudo cargar el archivo de configuracion.");
		exit(EXIT_FAILURE);
	}

	/*if (config_has_property(config, "JOB")) esto iria si el job lo hiciesemos local
		{
			job = config_get_int_value(config, "JOB");
		}*/

	clave = "JOB=";
	sJob = string_new();
	job++;
	string_append(&sJob,string_itoa(job));
	config_set_value(config,"JOB",sJob);
	//string_append(&sJob,"\n");
	fputs(clave, fpConfig);
	estadoGuardado = fputs(sJob, fpConfig);

	if (estadoGuardado==(EOF))
	{
		perror("[ERROR]: Fallo al guardar YAMAconfig.cfg");
	}

	if (config_has_property(config,"ALGORITMO_BALANCEO"))
		algoritmo = config_get_int_value(config,"ALGORITMO_BALANCEO");

	fclose(fpConfig);
	free(pathArchConfig);
	free(sJob);
	free(ruta);
	//config_destroy(config);
	return;
}


/*RPPL				           Re Pre Planificacion 	      					*/


void rePrePlanificacion(char *archivoTrabajo, char *archivoGuardadoFinal,char *archivoTMP,t_job* jobRePlanificado)
{
	t_tabla_estados *registroEstados;
	t_transformacionMaster *transformacion;
	t_list *listaTareas,*listaTransformacionesCaidas,
			*listaTransformacionesReplanificadas;
	int i,cantNodosInvolucrados;
	t_bloqueRecv *registroBloque;
	t_pedidoTransformacion pedidoTransformacion;

	crearListas();
	listaTareas = list_create();
	listaTransformacionesCaidas = list_create();
	listaTransformacionesReplanificadas = list_create();

	registroEstados = encontrarRegistro(archivoTMP);

	/* Prepara pedido de bloques a filesystem */
	pedidoTransformacion = prepararPedidoTransformacion(archivoTrabajo,archivoGuardadoFinal);

	/* Conseguir todas las tareas y las transformaciones del nodo caido */
	filtrarTareas(registroEstados->job,registroEstados->nodo,listaTareas,listaTransformacionesCaidas);

	/* Deshabilito nodo caido */
	workers[registroEstados->nodo].habilitado=0;

	/* Adquirir composicion del archivo*/
	envioPedidoArchivoAFS(pedidoTransformacion);

	recibirComposicionArchivo();


	/* Ordenar lista por mayor disponibilidad */
	list_sort(listaNodosInvolucrados,ordenarPorDisponibilidad);

	/* Generar vector que sera utilizado por el clock */
	cantNodosInvolucrados= list_size(listaNodosInvolucrados);
	int nodosInvolucrados[cantNodosInvolucrados];
	cargarVector(nodosInvolucrados,listaNodosInvolucrados);

			/*			Inicio rePlanificacion				*/

	//Planificacion de las transformaciones caidas en nodos alternos
	//No se utiliza clock o w-clock, no tiene sentido
	for (i=0;i<list_size(listaTransformacionesCaidas);i++)
	{
		transformacion = malloc(sizeof(t_transformacionMaster));
		registroEstados = list_get(listaTransformacionesCaidas,i);
		registroBloque = obtenerInfoBloque(registroEstados);
		cargarInfoTransformacionMaster(registroBloque,transformacion,jobRePlanificado->job);
		list_add(listaPlanTransformaciones,transformacion);//todas para resto de planificacion
		list_add(listaTransformacionesReplanificadas,transformacion);//solo caidas
	}

	cargarRestoTransformaciones(listaTareas,listaTransformacionesReplanificadas);

	/* Si la reduccion local ya esta terminada y hay una nueva transformacion en ese nodo
	 * la misma sera pisada cuando master informe la terminaciones de las nuevas transformaciones
	 * caso contrario no se volvera a realizar */
	planificacionReduccionLocal();

	/* La planificacion de reduccion global siempre se pisa, no importa si el nodo era el encargado
	 * o no ya que hay que pasarle la ubicacion de los nuevos archivos temporales de
	 * reduccion locales*/

	rePlanificacionReduccionGlobal(listaTareas,cantNodosInvolucrados,nodosInvolucrados);

	/*Limpieza*/

	destruir_listas();

	list_destroy(listaTareas);
	list_destroy(listaTransformacionesCaidas);
	list_destroy(listaTransformacionesReplanificadas);

	return;
}

void rePlanificacionReduccionGlobal(t_list *tareas, int cantNodosInvolucrados,int *nodosInvolucrados)
{
	t_tabla_estados *registro;
	int i;

	for(i=0;i<list_size(tareas);i++)
	{
		registro= list_get(tareas,i);

		if (registro->etapa==REDUCCION_GLOBAL)
		{
			registro->estado=ERROR_TAREA;
		}
	}

	planificacionReduccionGlobal(cantNodosInvolucrados,nodosInvolucrados);
	return;
}


void cargarRestoTransformaciones(t_list *tareas,t_list *listaTransformacionesReplanificadas)
{
	t_transformacionMaster *transformacion;
	t_bloqueRecv *bloqueRecibido;
	int i;

	for(i=0;i<list_size(listaBloquesRecibidos);i++)
	{
		bloqueRecibido = list_get(listaBloquesRecibidos,i);

		if(!transformacionCargada(listaTransformacionesReplanificadas,bloqueRecibido))
		{
			transformacion = recuperarInformacionTransformacion(tareas,bloqueRecibido);
			list_add(listaPlanTransformaciones,transformacion);
		}
	}
	return;
}

int transformacionCargada(t_list *listaTransformacionesReplanificadas,t_bloqueRecv *bloqueRecibido)
{
	t_transformacionMaster *registro;
	int i;

	for(i=0;i<list_size(listaTransformacionesReplanificadas);i++)
	{
		registro = list_get(listaTransformacionesReplanificadas,i);
		if(((registro->idNodo==bloqueRecibido->idNodo0)&&(registro->nroBloqueNodo==bloqueRecibido->nroBloqueNodo0))||
				((registro->idNodo==bloqueRecibido->idNodo1)&&(registro->nroBloqueNodo==bloqueRecibido->nroBloqueNodo1)))
		{
			return 1;
		}
	}
	return 0;
}

void cargarInfoTransformacionMaster(t_bloqueRecv *registroBloque,t_transformacionMaster *transformacion,int jobReplanificado)
{
	char *nombreTMP;
	int largoIp;

	if(workers[registroBloque->idNodo0].habilitado==1)
		{
			transformacion->idNodo=registroBloque->idNodo0;
			transformacion->nroBloqueNodo=registroBloque->nroBloqueNodo0;
		}
	else
		{
			transformacion->idNodo=registroBloque->idNodo1;
			transformacion->nroBloqueNodo=registroBloque->nroBloqueNodo1;
		}

	transformacion->bytesOcupados=registroBloque->bytesOcupados;
	transformacion->puerto=  workers[transformacion->idNodo].puerto;

	nombreTMP= generarNombreArchivoTemporal(jobReplanificado,transformacion->idNodo,transformacion->nroBloqueNodo);
	transformacion->largoArchivo = strlen(nombreTMP)+1;
	transformacion->archivoTransformacion=malloc(transformacion->largoArchivo);
	strcpy(transformacion->archivoTransformacion,nombreTMP);

	largoIp = strlen(workers[transformacion->idNodo].ip)+1;
	transformacion->largoIp=largoIp;
	strcpy(transformacion->ip,workers[transformacion->idNodo].ip);

	workers[transformacion->idNodo].workLoad++;

	return;

}

t_transformacionMaster *recuperarInformacionTransformacion(t_list* tareas,t_bloqueRecv *bloqueRecibido)
{
	t_transformacionMaster *transformacion;
	t_tabla_estados *registro;
	transformacion = malloc (sizeof(t_transformacionMaster));
	int i;

	for (i=0;i<list_size(tareas);i++)
	{
		registro = list_get(tareas,i);

		if((registro->nroBloqueArch==bloqueRecibido->nroBloqueArch)&&(registro->estado!=ERROR_TAREA))
		{
			transformacion->idNodo = registro->nodo;
			transformacion->nroBloqueNodo = registro->bloque;
			transformacion->bytesOcupados=bloqueRecibido->bytesOcupados;

			//nombre de archivo temporal
			transformacion->largoArchivo = strlen(registro->archivoTemp)+1;
			transformacion->archivoTransformacion= malloc(sizeof(transformacion->largoArchivo));
			strcpy(transformacion->archivoTransformacion,registro->archivoTemp);

			//Datos Ip y puerto
			transformacion->puerto=workers[registro->nodo].puerto;
			transformacion->largoIp = strlen(workers[registro->nodo].ip)+1;
			transformacion->ip=malloc(transformacion->largoIp);
			strcpy(transformacion->ip,workers[registro->nodo].ip);
		}
	}

	return transformacion;
}

t_bloqueRecv* obtenerInfoBloque(t_tabla_estados *registroEstados)
{
	int i;
	t_bloqueRecv *infoBloque;

	for (i=0;i<list_size(listaBloquesRecibidos);i++)
	{
		infoBloque = list_get(listaBloquesRecibidos,i);

		if(infoBloque->nroBloqueArch==registroEstados->nroBloqueArch)
		{
			return infoBloque;
		}
	}

	return infoBloque;

}

t_pedidoTransformacion prepararPedidoTransformacion(char *original,char *guardadoFinal)
{
	t_pedidoTransformacion pedidoTransformacion;
	int largoArchivo;

	largoArchivo =  strlen(original)+1;

	pedidoTransformacion.largoArchivo = largoArchivo;
	pedidoTransformacion.nombreArchivo= malloc(largoArchivo);
	strcpy(pedidoTransformacion.nombreArchivo,original);

	largoArchivo = strlen(guardadoFinal)+1;

	pedidoTransformacion.largoArchivo2 = largoArchivo;
	pedidoTransformacion.nombreArchivoGuardadoFinal = malloc(largoArchivo);
	strcpy(pedidoTransformacion.nombreArchivoGuardadoFinal,guardadoFinal);

	return pedidoTransformacion;
}

void filtrarTareas(int jobCaido,int nodoCaido,t_list* listaTareas,t_list *transformacionesCaidas){

	int i;
	t_tabla_estados *registro;

	for (i=0;i<list_size(listaTablaEstados);i++)
	{
		registro = list_get(listaTablaEstados,i);

		if(registro->job==jobCaido)
		{
			registro->estado=ERROR_TAREA;
			list_add(listaTareas,registro);//todas las tareas de ese trabajo y nodo
		}

		if((registro->nodo==nodoCaido)&&(registro->job==jobCaido)&&(registro->etapa==TRANSFORMACION))
			list_add(transformacionesCaidas,registro);//solo las transformaciones de ese nodo
	}

	return;
}


/*          Envio peticion a FS para iniciar operacion de planificacion      */

void envioPedidoArchivoAFS(t_pedidoTransformacion pedido){

	t_header *header;
	void *paquete,*bufferMensaje;
	int bytesAEnviar;

	header = malloc(sizeof(t_header));

	header->id=5;

	paquete = serializarPeticionInfoArchivo(pedido,header);

	bytesAEnviar = header->tamanioPayload;

	bufferMensaje = malloc(bytesAEnviar);

	/* Mensaje para FS */
	memcpy(bufferMensaje,header,sizeof(t_header));
	memcpy(bufferMensaje+sizeof(t_header),paquete,header->tamanioPayload);

	enviarPorSocket(socketFS,bufferMensaje,bytesAEnviar);

	free(bufferMensaje);
	free(paquete);
	return;
}

/*  						Recibir composicion de archivo   				*/

void recibirComposicionArchivo(){

	t_header *header;
	void *buffer;
	int cantidadDeBloques,i,desplazamiento=0;
	t_bloqueRecv *bloqueRecibido;

	header = malloc(sizeof(t_header));
	recibirHeader(socketFS,header);

	buffer = malloc(header->tamanioPayload);
	recibirPorSocket(socketFS,buffer,header->tamanioPayload);

	memcpy(&cantidadDeBloques,buffer,sizeof(uint32_t));
	desplazamiento+=sizeof(uint32_t);

	for (i=0;i<cantidadDeBloques;i++)
	{
		bloqueRecibido = malloc(sizeof(t_bloqueRecv));
		memcpy(&bloqueRecibido->nroBloqueArch,buffer+desplazamiento,sizeof(uint32_t));
		desplazamiento+=sizeof(uint32_t);

		memcpy(&bloqueRecibido->idNodo0,buffer+desplazamiento,sizeof(uint32_t));
		desplazamiento+=sizeof(uint32_t);

		memcpy(&bloqueRecibido->nroBloqueNodo0,buffer+desplazamiento,sizeof(uint32_t));
		desplazamiento+=sizeof(uint32_t);

		memcpy(&bloqueRecibido->idNodo1,buffer+desplazamiento,sizeof(uint32_t));
		desplazamiento+=sizeof(uint32_t);

		memcpy(&bloqueRecibido->nroBloqueNodo1,buffer+desplazamiento,sizeof(uint32_t));
		desplazamiento+=sizeof(uint32_t);

		memcpy(&bloqueRecibido->bytesOcupados,buffer+desplazamiento,sizeof(uint32_t));
		desplazamiento+=sizeof(uint32_t);

		guardarEnBloqueRecibidos(bloqueRecibido);

	}

	free(buffer);
	return;
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

	enviarPorSocket(jobMaster->socketMaster,bufferMensaje, 0);

	free(bufferMensaje);
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
	header.id = 9;
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

	enviarPorSocket(jobMaster->socketMaster,bufferMensaje, tamanioTotalBuffer);

	printf("envia planificacion a master\n");

	free(bufferTransformaciones);
	free(bufferRedLocal);
	free(bufferRedGlobal);


	free(bufferMensaje);
}


/*SRLZ					Serializaciones								 */

void* serializarTransformaciones(int cantTransformaciones, int* largoMensaje, t_list* lista){
	void* buffer;
	int i, desplazamiento = 0;
	t_transformacionMaster* transformacion;
	//transformacion= malloc(sizeof(t_transformacionMaster));
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
	//free(transformacion);
	return buffer;
}

void* serializarRedLocales(int cantReducciones, int* largoMensaje, t_list* lista){
	void* buffer;
	int i, desplazamiento = 0;
	t_reduccionLocalMaster *redLocal;
	//t_reduccionLocalMaster* redLocal = malloc(sizeof(t_reduccionLocalMaster));

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
	//free(redLocal);
	return buffer;
}

void* serializarRedGlobales(int cantReducciones, int* largoMensaje, t_list* lista){
	void* buffer;
	int i, desplazamiento = 0;
	t_reduccionGlobalMaster *redGlobal;
	//t_reduccionGlobalMaster* redGlobal = malloc(sizeof(t_reduccionGlobalMaster));

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
	//free(redGlobal);
	return buffer;
}

void* serializarPeticionInfoArchivo(t_pedidoTransformacion pedido, t_header* header)
{
	int desplazamiento=0;
	void* paquete;
	//Cualquier cosa revisar quilombo de doble puntero

	paquete= malloc(sizeof(uint32_t));

	//uint32_t *largoRuta=malloc(sizeof(uint32_t));
	memcpy(paquete,&pedido.largoArchivo,sizeof(uint32_t));
	desplazamiento+=sizeof(uint32_t);

	paquete = realloc(paquete,desplazamiento+pedido.largoArchivo);
	memcpy(paquete+desplazamiento,pedido.nombreArchivo,pedido.largoArchivo);
	desplazamiento+=pedido.largoArchivo;

	paquete = realloc(paquete,desplazamiento+(sizeof(uint32_t)));
	memcpy(paquete+desplazamiento,&pedido.largoArchivo2,sizeof(uint32_t));
	desplazamiento+=sizeof(uint32_t);

	paquete = realloc(paquete,desplazamiento+pedido.largoArchivo2);
	memcpy(paquete+desplazamiento,pedido.nombreArchivoGuardadoFinal,pedido.largoArchivo2);
	desplazamiento+=pedido.largoArchivo2;

	header->tamanioPayload = desplazamiento;

	return paquete;
}

