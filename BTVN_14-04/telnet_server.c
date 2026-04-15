#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>

#define MAX_CLIENTS 10
#define PORT 8080
#define BUFFER_SIZE 1024
#define DB_FILE "taikhoan.txt"

typedef enum { 
    STATE_WAIT_USER, 
    STATE_WAIT_PASS, 
    STATE_READY 
} ClientState;

typedef struct {
    int fd;
    ClientState state;
    char username[50];
} Client;

int check_login(const char *user, const char *pass) {
    FILE *fp = fopen(DB_FILE, "r");
    if (!fp) return 0; 

    char line[BUFFER_SIZE];
    char expected[BUFFER_SIZE];
    sprintf(expected, "%s %s", user, pass);

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (strcmp(expected, line) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

void execute_command_and_send(int client_fd, const char *cmd) {
    char out_filename[50], sys_cmd[BUFFER_SIZE + 100];
    sprintf(out_filename, "out_%d.txt", client_fd);
    sprintf(sys_cmd, "%s > %s 2>&1", cmd, out_filename);

    system(sys_cmd);

    FILE *fp = fopen(out_filename, "r");
    if (fp) {
        char buffer[BUFFER_SIZE];
        size_t bytes_read;
        
        while ((bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp)) > 0) {
            buffer[bytes_read] = '\0';
            send(client_fd, buffer, bytes_read, 0);
        }
        fclose(fp);
        remove(out_filename); 
    } else {
        send(client_fd, "Loi doc ket qua.\n", 17, 0);
    }

    char end_msg[] = "\n--- Lenh ket thuc ---\n";
    send(client_fd, end_msg, strlen(end_msg), 0);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    struct pollfd fds[MAX_CLIENTS + 1];
    Client clients[MAX_CLIENTS + 1];
    int nfds = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) exit(EXIT_FAILURE);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) exit(EXIT_FAILURE);
    if (listen(server_fd, 3) < 0) exit(EXIT_FAILURE);

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    printf("Telnet Server dang chay tai cong %d...\n", PORT);

    while (1) {
        if (poll(fds, nfds, -1) < 0) break;

        for (int i = 0; i < nfds; i++) {
            if (fds[i].fd == server_fd && (fds[i].revents & POLLIN)) {
                if (nfds < MAX_CLIENTS + 1) {
                    new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    if (new_socket >= 0) {
                        fds[nfds].fd = new_socket;
                        fds[nfds].events = POLLIN;
                        
                        clients[nfds].fd = new_socket;
                        clients[nfds].state = STATE_WAIT_USER;
                        memset(clients[nfds].username, 0, 50);
                        nfds++;

                        char prompt_user[] = "Moi nhap Username: ";
                        send(new_socket, prompt_user, strlen(prompt_user), 0);
                    }
                }
            } 
            else if (fds[i].fd > 0 && (fds[i].revents & POLLIN)) {
                char buffer[BUFFER_SIZE] = {0};
                int valread = recv(fds[i].fd, buffer, BUFFER_SIZE, 0);

                if (valread <= 0) { 
                    close(fds[i].fd);
                    for (int k = i; k < nfds - 1; k++) {
                        fds[k] = fds[k + 1];
                        clients[k] = clients[k + 1];
                    }
                    nfds--; i--; continue;
                }

                buffer[strcspn(buffer, "\r\n")] = 0;
                if (strlen(buffer) == 0 && clients[i].state != STATE_READY) continue; 

                if (clients[i].state == STATE_WAIT_USER) {
                    strncpy(clients[i].username, buffer, 49);
                    clients[i].state = STATE_WAIT_PASS;
                    
                    char prompt_pass[] = "Moi nhap Password: ";
                    send(fds[i].fd, prompt_pass, strlen(prompt_pass), 0);
                } 
                else if (clients[i].state == STATE_WAIT_PASS) {
                    if (check_login(clients[i].username, buffer)) {
                        clients[i].state = STATE_READY; 
                        char success[] = "Dang nhap thanh cong! Hay nhap lenh:\n";
                        send(fds[i].fd, success, strlen(success), 0);
                    } else {
                        char fail[] = "Dang nhap THAT BAI! Ket noi bi dong.\n";
                        send(fds[i].fd, fail, strlen(fail), 0);
                        
                        close(fds[i].fd);
                        for (int k = i; k < nfds - 1; k++) {
                            fds[k] = fds[k + 1];
                            clients[k] = clients[k + 1];
                        }
                        nfds--; i--; continue;
                    }
                } 
                else if (clients[i].state == STATE_READY) {
                    execute_command_and_send(fds[i].fd, buffer);
                }
            }
        }
    }
    close(server_fd);
    return 0;
}