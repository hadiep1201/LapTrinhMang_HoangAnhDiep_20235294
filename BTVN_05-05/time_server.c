#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 256

void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    free(arg); 

    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int read_size;

    printf("[+] Thread moi duoc tao cho Client (FD: %d)\n", client_socket);

    char *welcome_msg = "Ket noi thanh cong! Nhap lenh (VD: GET_TIME dd/mm/yyyy).\n";
    send(client_socket, welcome_msg, strlen(welcome_msg), 0);

    while ((read_size = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[read_size] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strlen(buffer) == 0) continue;

        printf("Client (FD %d) gui: %s\n", client_socket, buffer);

        if (strncmp(buffer, "GET_TIME ", 9) == 0) {
            char *format = buffer + 9; 
            time_t t = time(NULL);
            struct tm *tm_info = localtime(&t);

            if (strcmp(format, "dd/mm/yyyy") == 0) {
                strftime(response, sizeof(response), "%d/%m/%Y\n", tm_info);
            } else if (strcmp(format, "dd/mm/yy") == 0) {
                strftime(response, sizeof(response), "%d/%m/%y\n", tm_info);
            } else if (strcmp(format, "mm/dd/yyyy") == 0) {
                strftime(response, sizeof(response), "%m/%d/%Y\n", tm_info);
            } else if (strcmp(format, "mm/dd/yy") == 0) {
                strftime(response, sizeof(response), "%m/%d/%y\n", tm_info);
            } else {
                snprintf(response, sizeof(response), "Loi: Format khong duoc ho tro.\n");
            }
        } else {
            snprintf(response, sizeof(response), "Loi: Lenh khong hop le. Cu phap dung: GET_TIME [format]\n");
        }

        send(client_socket, response, strlen(response), 0);
    }

    if (read_size == 0) {
        printf("[-] Client (FD %d) da ngat ket noi.\n", client_socket);
    } else if (read_size == -1) {
        perror("Loi nhan du lieu");
    }

    close(client_socket);
    pthread_exit(NULL);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Khong the tao socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind that bai");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) < 0) {
        perror("Listen that bai");
        exit(EXIT_FAILURE);
    }

    printf("Time Server dang chay tren cong %d\n", PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Accept that bai");
            continue;
        }

        printf("[+] Co ket noi moi tu IP: %s, Port: %d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        int *new_sock = malloc(sizeof(int));
        *new_sock = client_socket;

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, (void *)new_sock) < 0) {
            perror("Khong the tao thread");
            free(new_sock);
            close(client_socket);
            continue;
        }

        pthread_detach(client_thread);
    }

    close(server_socket);
    return 0;
}