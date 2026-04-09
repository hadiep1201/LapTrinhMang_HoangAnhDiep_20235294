#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#include <ctype.h>

#define MAX_CLIENTS 1020
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    char id[50];
    char name[50];
    int is_authenticated;
} ClientInfo;

void removeClient(ClientInfo *clients, int *nClients, int i) {
    if (i < *nClients - 1) {
        clients[i] = clients[*nClients - 1];
    }
    *nClients -= 1;
}

int is_valid_name(const char *name) {
    for (int i = 0; name[i] != '\0'; i++) {
        if (isspace(name[i])) return 0;
        if (isalpha(name[i]) && !islower(name[i])) return 0;
    }
    return 1;
}

void get_timestamp(char *buf) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, 30, "%Y/%m/%d %I:%M:%S%p", t);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("Khoi tao socket that bai");
        return 1;
    }
    
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt that bai");
        close(listener);
        return 1;
    }
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);
    
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("Bind that bai");
        close(listener);
        return 1;
    }
    
    if (listen(listener, 5)) {
        perror("Listen that bai");
        close(listener);
        return 1;
    }
    
    printf("Server dang cho ket noi tai cong 8080...\n");
    
    ClientInfo clients[MAX_CLIENTS];
    int nClients = 0;
    fd_set fdread;
    char buf[BUFFER_SIZE];

    while (1) {
        FD_ZERO(&fdread);
        FD_SET(listener, &fdread);
        int maxdp = listener + 1;

        for (int i = 0; i < nClients; i++) {
            FD_SET(clients[i].fd, &fdread);
            if (maxdp < clients[i].fd + 1)
                maxdp = clients[i].fd + 1;
        }

        int ret = select(maxdp, &fdread, NULL, NULL, NULL);
        if (ret < 0) {
            perror("select that bai");
            break;
        }

        if (FD_ISSET(listener, &fdread)) {
            int new_fd = accept(listener, NULL, NULL);
            if (nClients < MAX_CLIENTS) {
                clients[nClients].fd = new_fd;
                clients[nClients].is_authenticated = 0;
                memset(clients[nClients].id, 0, 50);
                memset(clients[nClients].name, 0, 50);
                nClients++;
                char *welcome = "Vui long dinh danh: 'id: ten'\n";
                send(new_fd, welcome, strlen(welcome), 0);
            } else {
                char *msg = "Server da day.\n";
                send(new_fd, msg, strlen(msg), 0);
                close(new_fd);
            }
        }

        for (int i = 0; i < nClients; i++) {
            if (FD_ISSET(clients[i].fd, &fdread)) {
                int bytes_read = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);
                
                if (bytes_read <= 0) {
                    printf("Client (FD %d) da ngat ket noi\n", clients[i].fd);
                    close(clients[i].fd);
                    removeClient(clients, &nClients, i);
                    i--; 
                    continue;
                }

                buf[bytes_read] = '\0';
                buf[strcspn(buf, "\r\n")] = 0;

                if (clients[i].is_authenticated == 0) {
                    char temp_id[50], temp_name[50];
                    char *colon_ptr = strchr(buf, ':');
                    
                    if (colon_ptr != NULL) {
                        *colon_ptr = '\0';
                        strcpy(temp_id, buf);
                        sscanf(colon_ptr + 1, " %s", temp_name);

                        if (is_valid_name(temp_name)) {
                            strcpy(clients[i].id, temp_id);
                            strcpy(clients[i].name, temp_name);
                            clients[i].is_authenticated = 1;
                            char success_msg[100];
                            sprintf(success_msg, "Dinh danh thanh cong: %s\n", clients[i].name);
                            send(clients[i].fd, success_msg, strlen(success_msg), 0);
                        } else {
                            char err[] = "Loi: Ten phai viet thuong va khong dau cach.\n";
                            send(clients[i].fd, err, strlen(err), 0);
                        }
                    } else {
                        char err[] = "Loi: Sai cu phap. Hay dung 'id: ten'\n";
                        send(clients[i].fd, err, strlen(err), 0);
                    }
                } else {
                    char timestamp[30];
                    get_timestamp(timestamp);
                    char broadcast_msg[BUFFER_SIZE + 100];
                    sprintf(broadcast_msg, "%s %s: %s\n", timestamp, clients[i].id, buf);
                    
                    printf("[CHAT] %s", broadcast_msg);

                    for (int j = 0; j < nClients; j++) {
                        if (i != j && clients[j].is_authenticated) {
                            send(clients[j].fd, broadcast_msg, strlen(broadcast_msg), 0);
                        }
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}