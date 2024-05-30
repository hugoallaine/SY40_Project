#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define PORT 12345

int server_socket;
int *client_sockets;
int connected_clients = 0;

void handle_sigusr1(int sig) {
    for (int i = 0; i < connected_clients; ++i) {
        send(client_sockets[i], "Server shutting down. Closing connection.\n", 43, 0);
        close(client_sockets[i]);
    }
    close(server_socket);
    printf("Server shutdown complete.\n");
    exit(0);
}

void handle_sigusr2(int sig) {
    printf("Current connections: %d\n", connected_clients);
}

void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    free(arg);

    char buffer[1024];
    int bytes_read;

    while ((bytes_read = read(client_socket, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Client: %s\n", buffer);
        // Echo back to the client
        send(client_socket, buffer, bytes_read, 0);
    }

    close(client_socket);
    printf("Client disconnected.\n");
    return NULL;
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    pthread_t tid;

    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        addr_size = sizeof(client_addr);
        client_socket = malloc(sizeof(int));
        *client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (*client_socket < 0) {
            perror("Accept failed");
            free(client_socket);
            continue;
        }
        printf("Client connected.\n");

        if (pthread_create(&tid, NULL, handle_client, client_socket) != 0) {
            perror("Thread creation failed");
            free(client_socket);
        }
    }

    close(server_socket);
    return 0;
}