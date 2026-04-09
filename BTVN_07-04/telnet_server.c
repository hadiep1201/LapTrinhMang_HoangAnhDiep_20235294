#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <ctype.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef enum {
    STATE_WAIT_USER,
    STATE_WAIT_PASS,
    STATE_READY
} ClientState;

typedef struct {
    int fd;
    ClientState state;
    char username[50];
    char password[50];
} ClientInfo;

void removeClient(ClientInfo *clients, int *nClients, int i) {
    if (i < *nClients - 1) {
        clients[i] = clients[*nClients - 1];
    }
    *nClients -= 1;
}

int check_credentials(const char *user, const char *pass) {
    FILE *fp = fopen("users.txt", "r");
    if (fp == NULL) {
        perror("Khong the mo file users.txt");
        return 0;
    }

    char u[50], p[50];
    int found = 0;
    while (fscanf(fp, "%s %s", u, p) != EOF) {
        if (strcmp(u, user) == 0 && strcmp(p, pass) == 0) {
            found = 1;
            break;
        }
    }
    fclose(fp);
    return found;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind() failed");
        close(listener);
        return 1;
    }

    if (listen(listener, 5) < 0) {
        perror("listen() failed");
        close(listener);
        return 1;
    }

    printf("Telnet Server dang chay tai cong 8080...\n");

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
            perror("select() failed");
            break;
        }

        if (FD_ISSET(listener, &fdread)) {
            int new_fd = accept(listener, NULL, NULL);
            if (nClients < MAX_CLIENTS) {
                clients[nClients].fd = new_fd;
                clients[nClients].state = STATE_WAIT_USER;
                nClients++;
                char *msg = "Moi nhap Username: ";
                send(new_fd, msg, strlen(msg), 0);
            } else {
                char *msg = "Server day!\n";
                send(new_fd, msg, strlen(msg), 0);
                close(new_fd);
            }
        }

        for (int i = 0; i < nClients; i++) {
            if (FD_ISSET(clients[i].fd, &fdread)) {
                int bytes_read = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);

                if (bytes_read <= 0) {
                    close(clients[i].fd);
                    removeClient(clients, &nClients, i);
                    i--;
                    continue;
                }

                buf[bytes_read] = '\0';
                buf[strcspn(buf, "\r\n")] = 0;

                if (clients[i].state == STATE_WAIT_USER) {
                    strcpy(clients[i].username, buf);
                    clients[i].state = STATE_WAIT_PASS;
                    char *msg = "Moi nhap Password: ";
                    send(clients[i].fd, msg, strlen(msg), 0);

                } else if (clients[i].state == STATE_WAIT_PASS) {
                    strcpy(clients[i].password, buf);
                    if (check_credentials(clients[i].username, clients[i].password)) {
                        clients[i].state = STATE_READY;
                        char *msg = "Dang nhap thanh cong! Hay nhap lenh: \n";
                        send(clients[i].fd, msg, strlen(msg), 0);
                    } else {
                        char *msg = "Dang nhap THAT BAI! Ket noi bi dong.\n";
                        send(clients[i].fd, msg, strlen(msg), 0);
                        close(clients[i].fd);
                        removeClient(clients, &nClients, i);
                        i--;
                    }

                } else if (clients[i].state == STATE_READY) {
                    char command[BUFFER_SIZE + 10];
                     sprintf(command, "%s", buf);
                     FILE *fp = popen(command, "r");
                     if (fp != NULL) {
                        char line[BUFFER_SIZE];
                        while (fgets(line, sizeof(line), fp) != NULL) {
                            send(clients[i].fd, line, strlen(line), 0);
                        }
                        pclose(fp);
                        char *done_msg = "\n--- Lenh ket thuc ---\n";
                        send(clients[i].fd, done_msg, strlen(done_msg), 0);
                     } else {
                        char *err_msg = "Loi: Khong the thuc thi lenh!\n";
                        send(clients[i].fd, err_msg, strlen(err_msg), 0);
                     }
                    }
            }
        }
    }

    close(listener);
    return 0;
}