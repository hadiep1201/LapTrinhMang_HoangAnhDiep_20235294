#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define PORT 8080
#define NUM_WORKERS 5
#define BUFFER_SIZE 1024

void worker_process(int listener_socket) {
    char buf[BUFFER_SIZE];
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(listener_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("[Worker PID: %d] New client connected: %s:%d\n", 
               getpid(), inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        int ret = recv(client_socket, buf, sizeof(buf) - 1, 0);
        if (ret > 0) {
            buf[ret] = '\0';
            
            char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xin chao cac ban</h1></body></html>";
            
            send(client_socket, msg, strlen(msg), 0);
        }

        close(client_socket);
    }
}

int main() {
    int listener_socket;
    struct sockaddr_in server_addr;

    listener_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listener_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(listener_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(listener_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(listener_socket, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("HTTP Server dang chay tai cong %d\n", PORT);
    printf("Dang tao san %d tien trinh con (workers)...\n", NUM_WORKERS);

    for (int i = 0; i < NUM_WORKERS; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            printf(" -> Worker thu %d da san sang (PID: %d)\n", i + 1, getpid());
            worker_process(listener_socket);
            exit(0); 
        } 
        else if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_WORKERS; i++) {
        wait(NULL);
    }

    close(listener_socket);
    return 0;
}