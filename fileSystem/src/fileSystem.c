#include "funcionesFs.h"

int main(void) {

	ARCHCONFIG="fsConfig.cfg";
	cargarArchivoDeConfiguracion(ARCHCONFIG);

	int opt = TRUE;
	int master_socket , addrlen , new_socket , client_socket[30] , max_clients = 30 , activity, i  , sd;
    header header;
	int max_sd;
	void *buffer;
	struct sockaddr_in address;
	fd_set readfds;
	//Inicia todos los  client_socket[] en 0
	    for (i = 0; i < max_clients; i++)
	    {
	        client_socket[i] = 0;
	    }
	    //Creo el socket master
		if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
		{
			perror("Fallo creacion de socket master");
			exit(EXIT_FAILURE);
		}

	    //set master socket to allow multiple connections , this is just a good habit, it will work without this
		if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
		{
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}
		//Defino el tipo de socket con sus parametros.
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons( PUERTO );
		//bind al puerto indicado en la variable PUERTO. Cargada de la config O DEL ARCHIVO HEADER
		if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
		{
			perror("bind failed");
			exit(EXIT_FAILURE);
		}
		printf("\n Escuchando en el puerto: %d \n", PUERTO);

		//try to specify maximum of 3 pending connections for the master socket
		if (listen(master_socket, 3) < 0)
		{
			perror("Maximo de tres conexiones pendientes");
			exit(EXIT_FAILURE);
		}
		//Accepto conexion entrante
		    addrlen = sizeof(address);
		    puts("Esperando conexiones ...");
		    while(TRUE)
		    {
		        //Limpio la lista de sockets
		        FD_ZERO(&readfds);

		        //Agrego el socket master a la lista, para que tambien revise si hay cambios
		        FD_SET(master_socket, &readfds);
		        max_sd = master_socket;

		        //add child sockets to set
		        for ( i = 0 ; i < max_clients ; i++)
		        {
		            //socket descriptor
		            sd = client_socket[i];

		            //Si el sd es valido lo agrego a la lista de sockets
		            if(sd > 0)
		                FD_SET( sd , &readfds);

		            //highest file descriptor number, need it for the select function
		            if(sd > max_sd)
		                max_sd = sd;
		        }
		        //Espero que haya actividad en los sockets. Tiempo de espera null, nunca termina.
		        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

		        if (activity < 0)
		        //&& (errno!=EINTR))
		        {
		            printf("select error");
		        }

		        //Si algo cambio en el socket master, es una conexion entrante
		        if (FD_ISSET(master_socket, &readfds))
		        {
		            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
		            {
		                perror("accept");
		                exit(EXIT_FAILURE);
		            }
		            printf("Cliente conectado. IP: %s\n", (char*)inet_ntoa(address.sin_addr));
		            for (i = 0; i < max_clients; i++)
		            {
		                //Busco una pos vacia en la lista de clientes para guardar el socket entrante
		                if( client_socket[i] == 0 )
		                {
		                    client_socket[i] = new_socket;
		                    printf("Se agrego el nuevo socket en la posicion: %d\n" , i);

		                    break;
		                }
		            }
		        }
		        //else  es un cambio en los sockets que estaba escuchando.
		        for (i = 0; i < max_clients; i++)
		        {
		            sd = client_socket[i];
		            if (FD_ISSET( sd , &readfds))
		            {
		                //Chequea si fue para cerrarse y sino lee el mensaje;
		            	 if (recibirHeader(sd,&header) == 0)//desconexion
		                {
		                    //Alguien se desconecto.
		                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
		                    printf("Cliente desconectado sd: %d \n",sd);
		                    //Cierro el socket y pongo en cero la pos en la lista para reusarlo
		                    close( sd );
		                    client_socket[i] = 0;
		                }
		                else
		                {
		                	buffer=recibirPaquete(sd,header);
		                	//printf("Paquete recibido. \n Mensaje: %s usuario: %s \n ",nuevoPacketeRecbido.message,nuevoPacketeRecbido.username);
		                	procesarMensaje(buffer,sd,header);
		                	//free(buffer);
		                }
		            }
		        }
		    }
	return EXIT_SUCCESS;
}
