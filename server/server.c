#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define PORT 8000
#define MAX_CLIENTS 30

int clients[MAX_CLIENTS];
int num_clients = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_client(int client_socket) {
    char buffer[1024];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int read_size = recv(client_socket, buffer, sizeof(buffer), 0);
        if (read_size <= 0) {
            break;
        }
        printf("[BORADCAST] Cliente [ %d ]: %s\n", client_socket, buffer);

        pthread_mutex_lock(&mutex);
        for (int i = 0; i < num_clients; i++) {
            if (clients[i] != client_socket) {
                send(clients[i], buffer, strlen(buffer), 0);
            }
        }
        pthread_mutex_unlock(&mutex);
    }

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < num_clients; i++) {
        if (clients[i] == client_socket) {
            clients[i] = clients[num_clients-1];
            num_clients--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    printf("Cliente desconectado: %d\n", client_socket);
    close(client_socket);
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error al crear el socket del servidor");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Error al configurar la opción SO_REUSEADDR");
        close(server_socket);
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al enlazar");
        close(server_socket);
        exit(1);
    }

    listen(server_socket, 1);
    printf("\nEsperando conexiones en el puerto: %d\n\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_size);
        if (client_socket < 0) {
            perror("Error al aceptar la conexión");
            continue;
        }

        pthread_mutex_lock(&mutex);
        if (num_clients < MAX_CLIENTS) {
            clients[num_clients] = client_socket;
            num_clients++;
            printf("Cliente conectado desde %s\n\n", inet_ntoa(client_addr.sin_addr));
            pthread_t client_thread;
            pthread_create(&client_thread, NULL, (void *)handle_client, (void *)client_socket);
            pthread_detach(client_thread);
        } else {
            printf("Demasiados clientes conectados. Rechazando conexión de %s\n\n", inet_ntoa(client_addr.sin_addr));
            close(client_socket);
        }
        pthread_mutex_unlock(&mutex);
    }

    close(server_socket);
    return 0;
}