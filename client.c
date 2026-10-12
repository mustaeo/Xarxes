// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define DEFAULT_PORT 8080
#define DEFAULT_DOMAIN "localhost"
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
	int sock = 0;
	struct sockaddr_in serv_addr;
	char buffer[BUFFER_SIZE] = {0};
	char cadena[BUFFER_SIZE] = "";

	char *domini;
	int port;
	char *endptr;

	if (argc == 1) {
		// Cap argument -> valors per defecte
		domini = DEFAULT_DOMAIN;
		port = DEFAULT_PORT;
		printf("Cap argument indicat. Utilitzant valors per defecte: %s %d\n", domini, port);
	} 
	else if (argc == 3) {
		domini = argv[1];

		errno = 0;
		port = strtol(argv[2], &endptr, 10);

		// Comprovacions de validesa
		if (errno != 0 || *endptr != '\0' || port < 1 || port > 65535) {
			fprintf(stderr, "Error: el port ha de ser un enter entre 1 i 65535.\n");
			exit(EXIT_FAILURE);
		}
	} 
	else {
		fprintf(stderr, "Ús: %s [nom_de_domini port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}



	// Crear el socket
	// Afegiu comentari explicant els arguments, i de quines altres opcions hi ha
	// per al segon d'ells (ara SOCK_STREAM)
	
    //Socket() crea un socket. AF_INET indica IPv4. SOCK_STREAM indica TCP.
    //0 indica que utilitzarem el protocol per defecte d'aquest tipus de socket.
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\nError en crear el socket\n");
		return -1;
	}

	// Afegiu comentari sobre què són aquests 3 paràmetres,
	// per què s'utilitzen els valors que hi ha,
	// i per què s'utilitza htons()

    //sin_family indica la familia d'adreçes utilitzada (IPv4)
    //sin_port indica el port del servidor que pasarem amb htons() a l'ordre de bytes de la xarxa
    //inet_pton converteix una adreça IPv4 en format text a format binari i la guarda a sin_addr
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	inet_pton(AF_INET, domini, &serv_addr.sin_addr);	// Convertir adreça IPv4 a binari
   

	// Connectar amb el servidor
	// Afegiu comentari explicant els arguments

    //connect establece la conexion con el servidor.
    //Sock es el socket anteriormente creado, (struct sockadr*)&serv_addr nos dice la direccion
    // y el puerto del servidor, sizeof(serv_addr) indica la longitud de esta misma estrcutura.
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("\nConnexio fallida\n");
		return -1;
	}

	printf("Connectat al servidor. Podeu començar a enviar missatges.\n");

	int option;

	while (1) {
		memset(buffer, 0, sizeof(buffer)); // Netegem el buffer

		// Menú principal
		// Cal que implementeu un petit servei remot amb almenys 4 funcionalitats noves
		// Definiu vosaltres mateixos les dades a enviar (demanar en el client, i analitzar al servidor) i rebre
		// Si per donar més sentit al servei cal alguna opció més, la podeu afegir
		// Deixeu l'opció 1 com a Enviar missatge i l'última per Sortir

		printf("Menú principal:\n");
		printf("1. Enviar missatge\n");
		printf("2. Registrar producto\n");
		printf("3. Consultar producto\n");
		printf("4. Anadir unidades\n");
		printf("5. Retirar unidades\n");
		printf("6. Sortir\n");
		printf("Opció: ");

		scanf("%d", &option);
		while (getchar() != '\n');  // buida el buffer fins al salt de línia

		switch (option)
		{
		case 1:
		case 6:
			if (option == 1) {
				printf("Introdueix el missatge a enviar ('EXIT' per tancar el servidor i sortir): ");
				fgets(cadena, BUFFER_SIZE, stdin);
				cadena[strcspn(cadena, "\n")] = '\0';
			} else {
				strcpy(cadena, "EXIT");
			}
			// Primero enviamos la opcion.
			char seleccion;
			if (option == 1) {
				seleccion = '1';
			}
			else {
				seleccion = '6';
			}
			send(sock, &seleccion, 1, 0);
			// Despues enviamos el mensaje.
			send(sock, cadena, strlen(cadena) + 1, 0);
			// Recibir respuesta del servidor.
			memset(buffer, 0, BUFFER_SIZE);
			recv(sock, buffer, BUFFER_SIZE - 1, 0);
			printf("Resposta del servidor: %s\n", buffer);
			if (strcmp(cadena, "EXIT") == 0) {
				close(sock);
				return 0;
			}
			break;

		case 2:
			// Implementar Opció 2
			char datos[200];
			printf("Escribe ID|nombre|cantidad: ");
			if(fgets(datos, sizeof(datos), stdin)==NULL){
				close(sock);
				return 0;
			}

			if(strchr(datos, '\n')==NULL){
				int c;
				while((c=getchar()) != '\n' && c != EOF);
				printf("Entrada demasiado larga o incompleta.\n");
				break;
			}
			datos[strcspn(datos,"\n")] = '\0';

			seleccion = '2';
			int enviados = send(sock, &seleccion, 1, 0);
			if(enviados != 1){
				printf("No se pudo enviar la opcion.\n");
				close(sock);
				return 1;
			}

			int longitud = strlen(datos)+1;
			enviados = send(sock, datos, longitud, 0);
			if(enviados != longitud){
				printf("No se puedieron enviar todos los datos.\n");
				close(sock);
				return 1;
			}
			memset(buffer, 0, sizeof(buffer));
			int recibidos = recv(sock,buffer,sizeof(buffer) - 1, 0);
			if(recibidos<=0){
				printf("no se pudo recibir la respuesta.\n");
				close(sock);
				return -1;
			}
			printf("Respuesta del servidor: %s\n", buffer);
			break;

		case 3:{
			// Implementar Opció 3
			char datos[200];
			printf("Escribe el ID del producto: ");
			if(fgets(datos, sizeof(datos), stdin)==NULL){
				close(sock);
				return 0;
			}
			if(strchr(datos, '\n') == NULL){
				int c;
                while ((c = getchar()) != '\n' && c != EOF);
                printf("Entrada demasiado larga o incompleta.\n");
                break;
			}
			datos[strcspn(datos,"\n")] = '\0';
			char seleccion = '3';

			int enviados = send(sock, &seleccion, 1, 0);
			if(enviados != 1){
				printf("no se pudo enviar la opcion.\n");
				close(sock);
				return 1;
			}
			int longitud = strlen(datos) + 1;
			enviados = send(sock,datos,longitud,0);
			if(enviados != longitud){
				printf("no se pudo enviar el ID completo.\n");
				close(sock);
				return 1;
			}

			memset(buffer,0,sizeof(buffer));

			int recibidos = recv(sock, buffer, sizeof(buffer) -1, 0);
			if(recibidos <= 0){
				printf("No se pudo recibir la respuesta.\n");
				close(sock);
				return 1;
			}
			printf("Respuesta del servidor: %s\n", buffer);
			break;
		} 


		case 4:

			int id, cantidad;
			printf("Ingrese la ID y la cantidad a añadir (formato: ID/cantidad): ");
			
			mandar_info(sock, buffer, '4');

			break;

		case 5:
			id = 0;
			int cantidad_retirar = 0;

			printf("Ingrese la ID y la cantidad que desea retirar (formato: ID/cantidad): ");
			
			mandar_info(sock, buffer, '5');

			break;

		default:
			printf("Opció invàlida\n");
			break;
		}

	}

	close(sock);
	return 0;
}

void  mandar_info(int socket, char buff[], char seleccion) {
		
		char datos[200];
        int id, cant;

		if (scanf("%d|%d", &id, &cant) != 2 || cant <= 0) {
		printf("Entrada no válida: la cantidad debe ser mayor que 0.\n");
		return;
		}

		sprintf(datos, "%d|%d", id, cant);

		send(socket, &seleccion, 1, 0);
		send(socket, datos, strlen(datos)+1, 0);

		//poniendo datos del buffer a 0 antes de recibir
		memset(buff, 0, sizeof(buff));
		int recibido = recv(socket, buff, sizeof(buff)-1, 0);

		if (recibido <= 0) {
			printf("No se pudo recibir la respuesta del servidor.\n");
			close(socket);
			return;
		}

		buff[recibido] = '\0';
		printf("Respuesta del servidor: %s\n", buff);
	}