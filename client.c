/*
SY40 - Project | UTBM
@author: Hugo Allainé - Quentin Balezeau
client.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define PORT 12345
#define SERVER_IP "127.0.0.1"
#define NAME_SIZE 30

int keep_running = 1;
char client_name[30];

void handle_sigint(int sig) {
    keep_running = 0;
}

void *receive_messages(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[1024];
    int bytes_read;
   
    while ((bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        printf("\r\033[K");  // Clear the current line
        printf("- %s", buffer);
        printf("%s > ", client_name);
        fflush(stdout);
    }

    if (bytes_read == 0) {
        printf("\r\033[K");  // Clear the current line
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

    printf("Enter your name: ");
    fgets(client_name, sizeof(client_name), stdin);
    client_name[strcspn(client_name, "\n")] = 0; // remove newline character


    signal(SIGINT, handle_sigint);

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

    printf("Welcome %s, you are connected to server. Type your messages:\n", client_name);
    send(client_socket, client_name, strlen(client_name), 0);
    
    if (pthread_create(&receive_thread, NULL, receive_messages, &client_socket) != 0) {
        perror("Thread creation failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    while (keep_running) {
        printf("%s > ", client_name);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }
        if (!keep_running) break;
        send(client_socket, buffer, strlen(buffer), 0);
    }

    printf("\nDisconnecting...\n");
    close(client_socket);
    pthread_cancel(receive_thread);
    pthread_join(receive_thread, NULL);

    return 0;
}
