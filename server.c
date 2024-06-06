/*
SY40 - Project | UTBM
@author: Hugo Allainé - Quentin Balezeau
server.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define PORT 12345
#define NAME_SIZE 30

/**
 * @brief Structure to store client information
 * 
 * @param socket Client socket
 * @param ip Client IP address
 * @param name Client name
 * @param port Client port
 */
typedef struct {
    int socket;
    char ip[INET_ADDRSTRLEN];
    char name[NAME_SIZE];
    int port;
} client_info;

int server_socket; // Server socket
client_info client_sockets[MAX_CLIENTS]; // Array of client sockets
int connected_clients = 0; // Number of connected clients
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex to protect client_sockets

/**
 * @brief Function to handle SIGUSR1 signal
 * 
 * @param sig 
 */
void handle_sigusr1(int sig) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < connected_clients; ++i) {
        send(client_sockets[i].socket, "Server shutting down. Closing connection.\n", 43, 0);
        close(client_sockets[i].socket);
    }
    pthread_mutex_unlock(&clients_mutex);
    close(server_socket);
    printf("Server shutdown complete.\n");
    exit(0);
}

/**
 * @brief Function to handle SIGUSR2 signal
 * 
 * @param sig 
 */
void handle_sigusr2(int sig) {
    printf("Current connections: %d\n", connected_clients);
}

/**
 * @brief Function to broadcast a message to all connected clients except the sender
 * 
 * @param sender Client who sent the message
 * @param message Message to broadcast
 * @param message_len Length of the message
 */
void broadcast_message(const client_info *sender, const char *message, size_t message_len) {
    pthread_mutex_lock(&clients_mutex);
    char full_message[1024 + INET_ADDRSTRLEN + 10];
    snprintf(full_message, sizeof(full_message), "%s: %s",sender->name, message);
    for (int i = 0; i < connected_clients; ++i) {
        if (client_sockets[i].socket != sender->socket) {
            send(client_sockets[i].socket, full_message, strlen(full_message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

/**
 * @brief Function to handle client connection
 * 
 * @param arg Client information
 */
void *handle_client(void *arg) {
    client_info client = *((client_info *)arg);
    free(arg);

    char buffer[1024];
    int bytes_read;

    if ((bytes_read = read(client.socket, client.name, NAME_SIZE - 1)) > 0) {
        client.name[bytes_read] = '\0';
    } else {
        perror("Failed to read client name");
        close(client.socket);
        return NULL;
    }

    printf("Client %s (IP: %s:%d) connected.\n", client.name, client.ip, client.port);

    while ((bytes_read = read(client.socket, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Client %s (IP: %s:%d): %s", client.name, client.ip, client.port, buffer);
        broadcast_message(&client, buffer, bytes_read);
    }

    close(client.socket);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < connected_clients; ++i) {
        if (client_sockets[i].socket == client.socket) {
            client_sockets[i] = client_sockets[connected_clients - 1];
            connected_clients--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    printf("Client %s:%d disconnected.\n", client.ip, client.port);
    return NULL;
}

/**
 * @brief Main function
 * 
 * @return int 
 */
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
        int *client_socket = malloc(sizeof(int));
        *client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (*client_socket < 0) {
            perror("Accept failed");
            free(client_socket);
            continue;
        }

        if (*client_socket >= 0) {
            pthread_mutex_lock(&clients_mutex);
            if (connected_clients < MAX_CLIENTS) {
                client_info *client = malloc(sizeof(client_info));
                client->socket = *client_socket;
                inet_ntop(AF_INET, &client_addr.sin_addr, client->ip, INET_ADDRSTRLEN);
                client->port = ntohs(client_addr.sin_port);
                client_sockets[connected_clients++] = *client;
                pthread_mutex_unlock(&clients_mutex);
                if (pthread_create(&tid, NULL, handle_client, client) != 0) {
                    perror("Thread creation failed");
                    free(client);
                }
            } else {
                pthread_mutex_unlock(&clients_mutex);
                send(*client_socket, "Server is full. Try again later.\n", 33, 0);
                close(*client_socket);
                free(client_socket);
            }
        }
    }

    close(server_socket);
    return 0;
}