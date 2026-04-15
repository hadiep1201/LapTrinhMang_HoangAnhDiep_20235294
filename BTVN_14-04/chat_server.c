#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <ctype.h>
#include <time.h>

#define MAX_CLIENTS 10
#define PORT 8080
#define BUFFER_SIZE 1024

typedef enum { STATE_IDENTIFYING, STATE_CHATTING } ClientState;

typedef struct {
    int fd;
    char id[50];
    char name[50];
    ClientState state;
} Client;

// Kiểm tra xâu không có khoảng trắng
int is_no_spaces(const char *str) {
    if (strlen(str) == 0) return 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (isspace((unsigned char)str[i])) return 0;
    }
    return 1;
}

// Lấy thời gian hiện tại
void get_timestamp(char *buffer) {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, 26, "%Y/%m/%d %I:%M:%S%p", timeinfo);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    struct pollfd fds[MAX_CLIENTS + 1];
    Client clients[MAX_CLIENTS + 1];
    int nfds = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    printf("Chat Server dang chay tren port %d...\n", PORT);

    while (1) {
        int poll_count = poll(fds, nfds, -1);
        if (poll_count < 0) break;

        for (int i = 0; i < nfds; i++) {
            // 1. Xử lý kết nối mới
            if (fds[i].fd == server_fd && (fds[i].revents & POLLIN)) {
                if (nfds < MAX_CLIENTS + 1) {
                    new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    if (new_socket >= 0) {
                        fds[nfds].fd = new_socket;
                        fds[nfds].events = POLLIN;
                        clients[nfds].fd = new_socket;
                        clients[nfds].state = STATE_IDENTIFYING;
                        memset(clients[nfds].id, 0, 50);
                        memset(clients[nfds].name, 0, 50);
                        nfds++;
                        char msg[] = "Vui long dinh danh: 'client_id:client_name'\r\n";
                        send(new_socket, msg, strlen(msg), 0);
                    }
                }
            } 
            // 2. Xử lý dữ liệu từ client
            else if (fds[i].fd > 0 && (fds[i].revents & POLLIN)) {
                char buffer[BUFFER_SIZE] = {0};
                int valread = recv(fds[i].fd, buffer, BUFFER_SIZE, 0);

                if (valread <= 0) {
                    close(fds[i].fd);
                    for (int k = i; k < nfds - 1; k++) {
                        fds[k] = fds[k + 1];
                        clients[k] = clients[k + 1];
                    }
                    nfds--;
                    i--;
                    continue;
                }

                buffer[strcspn(buffer, "\r\n")] = 0;

                if (clients[i].state == STATE_IDENTIFYING) {
                    char *colon_ptr = strchr(buffer, ':');
                    if (colon_ptr) {
                        *colon_ptr = '\0';
                        char *temp_id = buffer;
                        char *temp_name = colon_ptr + 1;

                        while(isspace((unsigned char)*temp_name)) temp_name++;
                        char *end = temp_name + strlen(temp_name) - 1;
                        while(end > temp_name && isspace((unsigned char)*end)) {
                            *end = '\0';
                            end--;
                        }

                        if (is_no_spaces(temp_name)) {
                            strncpy(clients[i].id, temp_id, 49);
                            strncpy(clients[i].name, temp_name, 49);
                            clients[i].state = STATE_CHATTING;
                            char welcome[150];
                            sprintf(welcome, "Xac thuc thanh cong! Chao %s\nNhap tin nhan: ", clients[i].name);
                            send(fds[i].fd, welcome, strlen(welcome), 0);
                        } else {
                            char err[] = "Loi: Ten khong duoc co khoang trang!\n";
                            send(fds[i].fd, err, strlen(err), 0);
                        }
                    } else {
                        char err[] = "Loi: Sai dinh dang 'id: name'\n";
                        send(fds[i].fd, err, strlen(err), 0);
                    }
                } else {
                    // Xử lý chat (Broadcast)
                    char timestamp[30], broadcast_msg[BUFFER_SIZE + 100];
                    get_timestamp(timestamp);
                    sprintf(broadcast_msg, "[CHAT] %s %s: %s\n", timestamp, clients[i].id, buffer);

                    printf("%s", broadcast_msg);

                    for (int k = 1; k < nfds; k++) {
                        if (fds[k].fd != clients[i].fd) {
                            send(fds[k].fd, broadcast_msg, strlen(broadcast_msg), 0);
                        }
                    }
                }
            }
        }
    }
    close(server_fd);
    return 0;
}