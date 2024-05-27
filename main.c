#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define PORT 12345

int server_socket;
int client_sockets[];
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


int main() {

    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address));

    printf("Server is running on port %d\n", PORT);

    printf("Hello, World!\n");
    return 0;
}