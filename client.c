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

// Prototipo de la función para que main la conozca de antemano
void mandar_info(int socket, char buff[], int buff_size, char seleccion);

int main(int argc, char *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char cadena[BUFFER_SIZE] = "";

    char *domini;
    int port;
    char *endptr;

    if (argc == 1) {
        domini = DEFAULT_DOMAIN;
        port = DEFAULT_PORT;
        printf("Cap argument indicat. Utilitzant valors per defecte: %s %d\n", domini, port);
    } 
    else if (argc == 3) {
        domini = argv[1];

        errno = 0;
        port = strtol(argv[2], &endptr, 10);

        if (errno != 0 || *endptr != '\0' || port < 1 || port > 65535) {
            fprintf(stderr, "Error: el port ha de ser un enter entre 1 i 65535.\n");
            exit(EXIT_FAILURE);
        }
    } 
    else {
        fprintf(stderr, "Ús: %s [nom_de_domini port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nError en crear el socket\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, domini, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnexio fallida\n");
        return -1;
    }

    printf("Connectat al servidor. Podeu començar a enviar missatges.\n");

    int option;

    while (1) {
        memset(buffer, 0, sizeof(buffer));

        printf("Menú principal:\n");
        printf("1. Enviar missatge\n");
        printf("2. Registrar producto\n");
        printf("3. Consultar producto\n");
        printf("4. Anadir unidades\n");
        printf("5. Retirar unidades\n");
        printf("6. Sortir\n");
        printf("Opció: ");

        scanf("%d", &option);
        while (getchar() != '\n');

        switch (option) {
        case 1:
        case 6:
            if (option == 1) {
                printf("Introdueix el missatge a enviar ('EXIT' per tancar el servidor i sortir): ");
                fgets(cadena, BUFFER_SIZE, stdin);
                cadena[strcspn(cadena, "\n")] = '\0';
            } else {
                strcpy(cadena, "EXIT");
            }
            
            char seleccion;
            if (option == 1) {
                seleccion = '1';
            } else {
                seleccion = '6';
            }
            send(sock, &seleccion, 1, 0);
            send(sock, cadena, strlen(cadena) + 1, 0);
            
            memset(buffer, 0, BUFFER_SIZE);
            recv(sock, buffer, BUFFER_SIZE - 1, 0);
            printf("Resposta del servidor: %s\n", buffer);
            if (strcmp(cadena, "EXIT") == 0) {
                close(sock);
                return 0;
            }
            break;

        case 2: {
            char datos[200];
            printf("Escribe ID|nombre|cantidad: ");
            if (fgets(datos, sizeof(datos), stdin) == NULL) {
                close(sock);
                return 0;
            }

            if (strchr(datos, '\n') == NULL) {
                int c;
                while ((c = getchar()) != '\n' && c != EOF);
                printf("Entrada demasiado larga o incompleta.\n");
                break;
            }
            datos[strcspn(datos, "\n")] = '\0';

            seleccion = '2';
            int enviados = send(sock, &seleccion, 1, 0);
            if (enviados != 1) {
                printf("No se pudo enviar la opcion.\n");
                close(sock);
                return 1;
            }

            int longitud = strlen(datos) + 1;
            enviados = send(sock, datos, longitud, 0);
            if (enviados != longitud) {
                printf("No se pudieron enviar todos los datos.\n");
                close(sock);
                return 1;
            }
            memset(buffer, 0, sizeof(buffer));
            int recibidos = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (recibidos <= 0) {
                printf("No se pudo recibir la respuesta.\n");
                close(sock);
                return -1;
            }
            printf("Respuesta del servidor: %s\n", buffer);
            break;
        }
        case 3: {
            char datos[200];
            printf("Escribe el ID del producto: ");
            if (fgets(datos, sizeof(datos), stdin) == NULL) {
                close(sock);
                return 0;
            }
            if (strchr(datos, '\n') == NULL) {
                int c;
                while ((c = getchar()) != '\n' && c != EOF);
                printf("Entrada demasiado larga o incompleta.\n");
                break;
            }
            datos[strcspn(datos, "\n")] = '\0';
            char seleccion = '3';

            int enviados = send(sock, &seleccion, 1, 0);
            if (enviados != 1) {
                printf("No se pudo enviar la opcion.\n");
                close(sock);
                return 1;
            }
            int longitud = strlen(datos) + 1;
            enviados = send(sock, datos, longitud, 0);
            if (enviados != longitud) {
                printf("No se pudo enviar el ID completo.\n");
                close(sock);
                return 1;
            }

            memset(buffer, 0, sizeof(buffer));

            int recibidos = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (recibidos <= 0) {
                printf("No se pudo recibir la respuesta.\n");
                close(sock);
                return 1;
            }
            printf("Respuesta del servidor: %s\n", buffer);
            break;
        } 
        case 4:
            printf("Ingrese la ID y la cantidad a añadir (formato: ID/cantidad): ");
            mandar_info(sock, buffer, BUFFER_SIZE, '4');
            break;

        case 5:
            printf("Ingrese la ID y la cantidad que desea retirar (formato: ID/cantidad): ");
            mandar_info(sock, buffer, BUFFER_SIZE, '5');
            break;

        default:
            printf("Opció invàlida\n");
            break;
        }
    }

    close(sock);
    return 0;
}

void mandar_info(int socket, char buff[], int buff_size, char seleccion) {
    char datos[200];
    int id, cant;

    // Cambiado de "%d|%d" a "%d/%d" para que coincida con la barra que introduce el usuario
    if (scanf("%d/%d", &id, &cant) != 2 || cant <= 0) {
        printf("Entrada no válida: la cantidad debe ser mayor que 0.\n");
        while (getchar() != '\n'); // Limpiar búfer en caso de error
        return;
    }
    while (getchar() != '\n'); // Limpiar el salto de línea pendiente

    sprintf(datos, "%d/%d", id, cant);

    send(socket, &seleccion, 1, 0);
    send(socket, datos, strlen(datos) + 1, 0);

    memset(buff, 0, buff_size);
    int recibido = recv(socket, buff, buff_size - 1, 0);

    if (recibido <= 0) {
        printf("No se pudo recibir la respuesta del servidor.\n");
        close(socket);
        return;
    }

    buff[recibido] = '\0';
    printf("Respuesta del servidor: %s\n", buff);
}