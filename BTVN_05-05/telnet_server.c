#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_FD 1024 

typedef struct {
    int state;     
    char user[64];
} ClientState;

int check_login(char *username, char *password) {
    FILE *f = fopen("taikhoan.txt", "r");
    if (!f) return 0;

    char line[256], file_user[64], file_pass[64];
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%s %s", file_user, file_pass) == 2) {
            if (strcmp(username, file_user) == 0 && strcmp(password, file_pass) == 0) {
                fclose(f);
                return 1;
            }
        }
    }
    fclose(f);
    return 0;
}

void execute_command(int client_fd, char *cmd) {
    char sys_cmd[512];
    char out_file[64];
    
    sprintf(out_file, "out_%d.txt", client_fd);
    sprintf(sys_cmd, "%s > %s 2>&1", cmd, out_file);
    
    system(sys_cmd);

    FILE *f = fopen(out_file, "r");
    if (f) {
        char buf[1024];
        int bytes_read;
        while ((bytes_read = fread(buf, 1, sizeof(buf) - 1, f)) > 0) {
            buf[bytes_read] = '\0';
            send(client_fd, buf, bytes_read, 0);
        }
        fclose(f);
    } else {
        char *err = "Khong the doc ket qua lenh.\n";
        send(client_fd, err, strlen(err), 0);
    }
    
    remove(out_file);
}

int main() {
    int listener;
    struct sockaddr_in server_addr;
    fd_set master_set, read_set;
    int fd_max;
    
    ClientState clients[MAX_FD];
    memset(clients, 0, sizeof(clients));

    listener = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listener, 10);

    FD_ZERO(&master_set);
    FD_SET(listener, &master_set);
    fd_max = listener;

    printf("Telnet Server dang chay tai cong %d\n", PORT);

    while (1) {
        read_set = master_set;

        if (select(fd_max + 1, &read_set, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }

        for (int i = 0; i <= fd_max; i++) {
            if (FD_ISSET(i, &read_set)) {
                
                if (i == listener) {
                    struct sockaddr_in client_addr;
                    socklen_t addrlen = sizeof(client_addr);
                    int new_fd = accept(listener, (struct sockaddr *)&client_addr, &addrlen);
                    
                    FD_SET(new_fd, &master_set);
                    if (new_fd > fd_max) fd_max = new_fd;

                    clients[new_fd].state = 0;
                    
                    printf("[+] Co ket noi moi (FD: %d)\n", new_fd);
                    char *msg = "Welcome to Telnet Server!\nUsername: ";
                    send(new_fd, msg, strlen(msg), 0);
                } 
                
                else {
                    char buf[256];
                    int nbytes = recv(i, buf, sizeof(buf) - 1, 0);

                    if (nbytes <= 0) {
                        printf("[-] Client (FD: %d) ngat ket noi\n", i);
                        close(i);
                        FD_CLR(i, &master_set);
                    } else {
                        buf[nbytes] = '\0';
                        buf[strcspn(buf, "\r\n")] = 0; 
                        
                        if (strlen(buf) == 0) continue;

                        if (clients[i].state == 0) {
                            strncpy(clients[i].user, buf, sizeof(clients[i].user) - 1);
                            clients[i].state = 1;
                            char *msg = "Password: ";
                            send(i, msg, strlen(msg), 0);
                        } 
                        else if (clients[i].state == 1) {
                            if (check_login(clients[i].user, buf)) {
                                clients[i].state = 2;
                                char *msg = "Dang nhap thanh cong!\nCommand: ";
                                send(i, msg, strlen(msg), 0);
                            } else {
                                clients[i].state = 0;
                                char *msg = "Sai user hoac pass. Vui long dang nhap lai.\nUsername: ";
                                send(i, msg, strlen(msg), 0);
                            }
                        } 
                        else if (clients[i].state == 2) {
                            if (strcmp(buf, "exit") == 0) {
                                close(i);
                                FD_CLR(i, &master_set);
                                printf("[-] Client (FD: %d) da thoat\n", i);
                                continue;
                            }
                            
                            execute_command(i, buf);
                            
                            char *msg = "\nCommand: ";
                            send(i, msg, strlen(msg), 0);
                        }
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}