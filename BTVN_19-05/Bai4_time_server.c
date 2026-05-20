#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 9000
#define BUFFER_SIZE 1024

void* handle_client(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    char *welcome = "Ket noi thanh cong! Nhap lenh GET_TIME [format] de xem gio.\n";
    send(client_sock, welcome, strlen(welcome), 0);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes <= 0) break;

        buffer[strcspn(buffer, "\r\n")] = 0;
        
        if (strlen(buffer) == 0) continue;

        if (strncmp(buffer, "GET_TIME ", 9) == 0) {
            char *format_req = buffer + 9;
            char strftime_fmt[20] = "";

            if (strcmp(format_req, "dd/mm/yyyy") == 0) {
                strcpy(strftime_fmt, "%d/%m/%Y");
            } else if (strcmp(format_req, "dd/mm/yy") == 0) {
                strcpy(strftime_fmt, "%d/%m/%y");
            } else if (strcmp(format_req, "mm/dd/yyyy") == 0) {
                strcpy(strftime_fmt, "%m/%d/%Y");
            } else if (strcmp(format_req, "mm/dd/yy") == 0) {
                strcpy(strftime_fmt, "%m/%d/%y");
            }

            if (strlen(strftime_fmt) > 0) {
                time_t t = time(NULL);
                struct tm *tm = localtime(&t);
                char time_res[100];
                
                strftime(time_res, sizeof(time_res), strftime_fmt, tm);
                strcat(time_res, "\n");
                send(client_sock, time_res, strlen(time_res), 0);
            } else {
                char *err = "Loi: Dinh dang thoi gian khong ho tro!\n";
                send(client_sock, err, strlen(err), 0);
            }
        } else {
            char *err = "Loi: Sai cu phap. Vui long dung lenh: GET_TIME [format]\n";
            send(client_sock, err, strlen(err), 0);
        }
    }

    close(client_sock);
    return NULL;
}

int main() {
    int server_socket, *new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) exit(1);

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) exit(1);

    if (listen(server_socket, 10) == 0) {
        printf("Time Server dang chay tren cong %d...\n", PORT);
    } else {
        exit(1);
    }

    while (1) {
        addr_size = sizeof(client_addr);
        int client_sock = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        if (client_sock < 0) continue;

        printf("Client ket noi: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        new_socket = malloc(sizeof(int));
        *new_socket = client_sock;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)new_socket) != 0) {
            free(new_socket);
        } else {
            pthread_detach(thread_id);
        }
    }

    close(server_socket);
    return 0;
}