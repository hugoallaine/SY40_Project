#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"

void *receive_messages(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[1024];
    int bytes_read;

    while ((bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Server: %s\n", buffer);
    }

    if (bytes_read == 0) {
        printf("Connection closed by server.\n");
    } else {
        perror("recv failed");
    }

    close(client_socket);
    exit(0);
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[1024];
    pthread_t receive_thread;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server. Type your messages:\n");

    if (pthread_create(&receive_thread, NULL, receive_messages, &client_socket) != 0) {
        perror("Thread creation failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    while (1) {
        printf("> ");
        fgets(buffer, sizeof(buffer), stdin);
        send(client_socket, buffer, strlen(buffer), 0);
    }

    pthread_join(receive_thread, NULL);
    close(client_socket);
    return 0;
}
