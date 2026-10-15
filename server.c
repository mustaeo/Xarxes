// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 1024
#define MAX_PRODUCTOS 100

int info_recibida(const char *buffer, char ids[][50],
                  int *id, int *cant, int num_productos);

int main(int argc, char *argv[]) {
    int sock, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    int port;
    char *endptr;

    char ids[MAX_PRODUCTOS][50];
    char nombres[MAX_PRODUCTOS][100];
    int cantidades[MAX_PRODUCTOS];

    int num_productos = 0;

    if (argc == 1) {
        port = DEFAULT_PORT;
        printf("Cap port indicat. Utilitzant valor per defecte: %d\n", port);
    }
    else if (argc == 2) {
        errno = 0;
        port = strtol(argv[1], &endptr, 10);

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
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error en crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar dirección
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    // Bind
    if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error en fer el bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(sock, 3) < 0) {
        perror("Error en escoltar");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Evitar procesos zombis automáticamente al terminar los hijos
    signal(SIGCHLD, SIG_IGN);

    printf("Servidor en funcionament, esperant connexions...\n");

    while (1) {
        // Acceptar connexions de clients
        if ((new_socket = accept(sock, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Error en acceptar la connexió");
            continue;
        }

        // Crear un procés fill per gestionar el client de forma concurrent
        pid_t pid = fork();
        if (pid < 0) {
            perror("Error en crear el fork");
            close(new_socket);
            continue;
        }

        if (pid == 0) {
            // --- PROCÉS FILL ---
            close(sock); // El fill no necessita el socket principal d'escolta

            char buffer[BUFFER_SIZE] = {0};

            while (1) {
                char seleccion;
                memset(buffer, 0, BUFFER_SIZE);

                if (recv(new_socket, &seleccion, 1, 0) <= 0) {
                    printf("Client desconnectat (PID: %d)\n", getpid());
                    break;
                }
                
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
                    char *id_str = strtok(buffer, "|");
                    char *nombre = strtok(NULL, "|");
                    char *cantidad_texto = strtok(NULL, "|");

                    if(id_str == NULL || nombre == NULL || cantidad_texto == NULL){
                        send(new_socket, "Formato incorrecto\n", strlen("Formato incorrecto\n"), 0);
                    }
                    else{
                        int encontrado = 0;
                        for(int i=0; i < num_productos; i++){
                            if(strcmp(ids[i], id_str) == 0){
                                encontrado = 1;
                            }
                        }
                        if(encontrado == 1){
                            send(new_socket, "El producto ya existe\n", strlen("El producto ya existe\n"), 0);
                        }
                        else if(num_productos >= MAX_PRODUCTOS){
                            send(new_socket, "Inventario lleno\n", strlen("Inventario lleno\n"), 0);
                        }
                        else{
                            strcpy(ids[num_productos], id_str);
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
                    for(int i=0; i < num_productos; i++){
                        if(strcmp(ids[i], buffer) == 0){
                            posicion = i;
                        }
                    }
                    if(posicion == -1){
                        send(new_socket, "Producto no encontrado\n", strlen("Producto no encontrado\n"), 0);
                    }
                    else{
                        char respuesta[BUFFER_SIZE];
                        snprintf(respuesta, sizeof(respuesta), "ID: %s | Nombre: %s | Cantidad: %d\n", ids[posicion], nombres[posicion], cantidades[posicion]);
                        send(new_socket, respuesta, strlen(respuesta), 0);
                    }
                }
                // OPCION 4 - AÑADIR UNIDADES
                else if (seleccion == '4'){
                    int id_prod, cantidad;
                    int posProd = info_recibida(buffer, ids, &id_prod, &cantidad, num_productos);

                    if (posProd == -2) {
                        send(new_socket, "Formato o cantidad invalidos\n", strlen("Formato o cantidad invalidos\n"), 0);
                    }
                    else if (posProd == -1) {
                        send(new_socket, "Producto no encontrado\n", strlen("Producto no encontrado\n"), 0);
                    }
                    else {
                        cantidades[posProd] += cantidad;
                        send(new_socket, "Cantidad añadida\n", strlen("Cantidad añadida\n"), 0);
                    }
                }
                // OPCION 5 - RETIRAR UNIDADES
                else if (seleccion == '5'){
                    int id_prod, cantidad_retirar;
                    int posProd = info_recibida(buffer, ids, &id_prod, &cantidad_retirar, num_productos);

                    if (posProd == -2) {
                        send(new_socket, "Formato o cantidad invalidos\n", strlen("Formato o cantidad invalidos\n"), 0);
                    }
                    else if (posProd == -1) {
                        send(new_socket, "Producto no encontrado\n", strlen("Producto no encontrado\n"), 0);
                    }
                    else {
                        cantidades[posProd] -= cantidad_retirar;
                        send(new_socket, "Cantidad retirada\n", strlen("Cantidad retirada\n"), 0);
                    }
                }
                // OPCION 6 - SALIR
                else if (seleccion == '6' || strcmp(buffer, "EXIT") == 0) {
                    printf("Tancant connexió amb el client (PID: %d)...\n", getpid());
                    send(new_socket, "Connexió tancada\n", strlen("Connexió tancada\n"), 0);
                    break;
                }
            }

            close(new_socket);
            exit(0); // El fill finalitza la seva execució aquí
        } 
        else {
            // --- PROCÉS PARE ---
            close(new_socket); // El pare tanca el socket del client i torna a l'accept
        }
    }

    close(sock);
    return 0;
}

int info_recibida(const char *buffer, char ids[][50],
                  int *id, int *cant, int num_productos) {
    if (sscanf(buffer, "%d/%d", id, cant) != 2 || *cant <= 0) {
        return -2;
    }

    for (int i = 0; i < num_productos; i++) {
        if (*id == atoi(ids[i])) {
            return i;
        }
    }
    return -1;
}