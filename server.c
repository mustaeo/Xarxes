// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 1024
#define MAX_PRODUCTOS 100

int info_recibida(const char *buffer, char ids[][50],
                  int *id, int *cant, int num_productos);

int main(int argc, char *argv[]) {
	int sock, new_socket;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	char buffer[BUFFER_SIZE] = {0};

	int port;
	char *endptr;

	char ids[MAX_PRODUCTOS][50];
	char nombres[MAX_PRODUCTOS][100];
	int cantidades[MAX_PRODUCTOS];

	int num_productos = 0;



	if (argc == 1) {
		// Cap argument -> port per defecte
		port = DEFAULT_PORT;
		printf("Cap port indicat. Utilitzant valor per defecte: %d\n", port);
	}
	else if (argc == 2) {
		errno = 0;
		port = strtol(argv[1], &endptr, 10);

		// Comprovacions
		if (errno != 0 || *endptr != '\0' || port < 1 || port > 65535) {
			fprintf(stderr, "Error: el port ha de ser un enter entre 1 i 65535.\n");
			exit(EXIT_FAILURE);
		}
	}
	else {
		fprintf(stderr, "Ús: %s [port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}


	// Crear el socket
	// Afegiu comentari explicant els arguments
	
    //Socket() crea un socket. AF_INET indica IPv4. SOCK_STREAM indica TCP.
    //0 indica que utilitzarem el protocol per defecte d'aquest tipus de socket.
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror("Error en crear el socket");
	exit(EXIT_FAILURE);
}


	// Afegiu comentari explicant què són aquests 3 paràmetres,
	// per què s'utilitzen htons() i htonl(),
	// i per què s'utilitzen els valors que hi ha

    ////sin_family indica la familia d'adreçes utilitzada (IPv4)
    //sin_port indica el port del servidor que pasarem amb htons() a l'ordre de bytes de la xarxa
    //inet_pton converteix una adreça IPv4 en format text a format binari i la guarda a sin_addr
    //htonl converteix un valor de 32 bits a l'ordre de bytes de xarxa
    //sin_port indica el port d'escolta, i htons el converteix a l'ordre de bytes de xarxa (16bits)
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);	// Per a INADDR_ANY no faria falta htonl(), però li posem per coherència
	address.sin_port = htons(port);

	// Enllaçar el socket al port especificat
	// Afegiu comentari explicant els arguments

    //bind associa el socket amb una adreça IP i un port, que estarian indicades a (struct sockaddr *)&address
    // i sizeof(address) indicaria la longitud de la estrucutura de la adreça.
	if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("Error en fer el bind");
		close(sock);
		exit(EXIT_FAILURE);
	}

	// Escoltar connexions entrants
	// Afegiu comentari explicant els arguments
	
    // listen posa el socket en mode escolta, 3 seria el maxim de cua
    // de conexions pendents
	if (listen(sock, 3) < 0) {
		perror("Error en escoltar");
		close(sock);
		exit(EXIT_FAILURE);
	}

	printf("Servidor en funcionament, esperant connexions...\n");

	while (1) {
		// Acceptar connexions de clients
		// Afegiu comentari explicant els arguments i per què hi ha i cal new_socket si ja tenim sock
		
        //accept accepta una connexio entrant d'un client.
        //sock es el socket anteriorment creat.
        //(struct sockaddr *)&address guarda l'adreça del client que es conecta
        //(socklen_t *)&addrlen indica la mida de l'estructura de l'adreça
        //Es crea un new_socket amb l'objectiu de que aquest atengui al client
        //mentres que el sock original segueix esperant per acceptar noves connexions
		if ((new_socket = accept(sock, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
			perror("Error en acceptar la connexió");
			close(sock);
			exit(EXIT_FAILURE);
		}

		 while (1) {

			char seleccion;
			// Primero limpiamos el buffer.
			memset(buffer, 0, BUFFER_SIZE);
			//El cliente envia primero un caracter:
			if (recv(new_socket, &seleccion, 1, 0) <= 0) {

				printf("Client desconnectat (PID: %d)\n", getpid());
				break;
			}
			
			// Despues recibimos los datos.
			if (recv(new_socket, buffer, BUFFER_SIZE - 1, 0) <= 0) {

				printf("Client desconnectat (PID: %d)\n", getpid());
				break;
			}
			printf("Missatge rebut del client (PID: %d): %s\n", getpid(), buffer);
			// OPCION 1 - ENVIAR MENSAJE
			if(seleccion == '1'){
				send(new_socket, "Missatge rebut\n", strlen("Missatge rebut\n"), 0);
			}
			// OPCION 2 - REGISTRAR PRODUCTO
			else if(seleccion == '2'){
				char *id = strtok(buffer, "|");
				char *nombre = strtok(NULL, "|");
				char *cantidad_texto = strtok(NULL, "|");
				// Comprobamos que las tres partes existen.
				if(id == NULL || nombre == NULL || cantidad_texto == NULL){
					send(new_socket, "Formato incorrecto\n", strlen("Formato incorrecto\n"), 0);
				}
				else{
					// Al principio suponemos que el producto
					// no existe.
					int encontrado = 0;
					// Recorremos los productos que ya tenemos.
					for(int i=0; i< num_productos; i++){
						// Si encontramos el mismo ID,
						// ponemos encontrado a 1.
						if(strcmp(ids[i],id)== 0){
							encontrado = 1;
						}
					}
					// Si ya existe, no lo registramos otra vez.
					if(encontrado == 1){
						send(new_socket, "El producto ya existe\n", strlen("El producto ya existe\n"), 0);
					}
					// Comprobamos que quede espacio.
					else if(num_productos>= MAX_PRODUCTOS){
						send(new_socket, "Inventario lleno\n", strlen("Inventario lleno\n"), 0);
					}
					// Si todo esta bien, guardamos el producto.
					else{
						strcpy(ids[num_productos],id);
						strcpy(nombres[num_productos], nombre);
						cantidades[num_productos] = atoi(cantidad_texto);
						num_productos++;
						send(new_socket, "Producto registrado\n", strlen("Producto registrado\n"), 0);
					}
				}
			}
			// OPCION 3 - CONSULTAR PRODUCTO
			else if(seleccion == '3'){
				int posicion = -1;
				// Buscamos el producto.
				for(int i=0; i<num_productos; i++){
					if(strcmp(ids[i], buffer) == 0){
						posicion = i;
					}
				}
				// Primero termina el for.
				// Despues comprobamos si lo hemos encontrado.
				if(posicion ==-1){
					send(new_socket,"Producto no encontrado\n", strlen("Producto no encontrado\n"),0);
				}
				else{
					char respuesta [BUFFER_SIZE];
					snprintf(respuesta, sizeof(respuesta), "ID: %s | Nombre: %s | Cantidad: %d\n", ids[posicion], nombres[posicion], cantidades[posicion]);
					send(new_socket, respuesta, strlen(respuesta), 0);
				}

			}

			else if (seleccion == '4'){
				
				int id, cantidad;
				int posProd = info_recibida(&buffer, ids, &id, &cantidad, num_productos);

				//checkeando si se encontro el producto

				if (posProd == -2) {
   					 send(new_socket, "Formato o cantidad invalidos\n",
         			strlen("Formato o cantidad invalidos\n"), 0);
				}
				else if (posProd == -1) {
					send(new_socket, "Producto no encontrado\n",
        			strlen("Producto no encontrado\n"), 0);
				}
				else {
					cantidades[posProd] += cantidad;
					send(new_socket, "Cantidad añadida\n", strlen("Cantidad añadida\n"), 0);
					}
			}

			else if (seleccion == '5'){

				int id, cantidad_retirar;
				int posProd = info_recibida(&buffer, ids, &id, &cantidad_retirar, num_productos);

				//checkeando si se encontro el producto

				if (posProd == -2) {
   					 send(new_socket, "Formato o cantidad invalidos\n",
         			strlen("Formato o cantidad invalidos\n"), 0);
				}
				else if (posProd == -1) {
					send(new_socket, "Producto no encontrado\n",
        			strlen("Producto no encontrado\n"), 0);
				}
				else {
					cantidades[posProd] += cantidad_retirar;
					send(new_socket, "Cantidad añadida\n", strlen("Cantidad añadida\n"), 0);
					}
			}

			// OPCION 6 - SALIR
			else if (seleccion == '6' || strcmp(buffer, "EXIT") == 0) {
				printf("Tancant connexió amb el client (PID: %d)...\n", getpid());
				send(new_socket, "Connexió tancada\n", strlen("Connexió tancada\n"), 0);
				break;
			}


			// Aquí haureu d'implementar l'anàlisi de la cadena rebuda per saber l'operació,
			// i si n'hi ha els arguments, executar-la i tornar el(s) resultat(s)

			// A continuació hi ha el codi corresponent a l'opció d'Enviar missatge, amb el retorn d'una cadena fixa,
			// i la de Sortida de la connexió/bucle si és el cas.
			// Modifiqueu la cadena de retorn per tal que sigui la que diu l'enunciat.
		}

		//funcion para mandar informacion

	

		close(new_socket); // Tancar la connexió amb el client
	}

	close(sock);
	return 0;
}

int info_recibida(const char *buffer, char ids[][50],
                  int *id, int *cant, int num_productos) {

    // Extraemos el ID y la cantidad del mensaje recibido.
    if (sscanf(buffer, "%d/%d", id, cant) != 2 || *cant <= 0) {
        return -2;
    }

    // Buscamos el ID dentro del inventario.
    for (int i = 0; i < num_productos; i++) {

        if (*id == atoi(ids[i])) {
            return i;
        }
    }

    // Si hemos recorrido todo el inventario y no existe el ID.
    return -1;
}