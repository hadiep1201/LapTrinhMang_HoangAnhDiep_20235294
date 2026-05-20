#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 9000
#define BUFFER_SIZE 1024
#define ACCOUNT_FILE "taikhoan.txt"

int check_login(char *username, char *password) {
    FILE *fp = fopen(ACCOUNT_FILE, "r");
    if (fp == NULL) return 0;

    char file_user[50], file_pass[50];
    while (fscanf(fp, "%s %s", file_user, file_pass) != EOF) {
        if (strcmp(username, file_user) == 0 && strcmp(password, file_pass) == 0) {
            fclose(fp);
            return 1; 
        }
    }
    fclose(fp);
    return 0; 
}

void* handle_client(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    char username[50], password[50];
    int is_logged_in = 0;

    while (!is_logged_in) {
        char *user_prompt = "Username: ";
        send(client_sock, user_prompt, strlen(user_prompt), 0);

        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            close(client_sock);
            return NULL;
        }
        
        if (sscanf(buffer, "%s", username) != 1) {
            send(client_sock, "Vui long khong de trong!\n", 25, 0);
            continue;
        }

        char *pass_prompt = "Password: ";
        send(client_sock, pass_prompt, strlen(pass_prompt), 0);

        memset(buffer, 0, BUFFER_SIZE);
        bytes = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            close(client_sock);
            return NULL;
        }

        if (sscanf(buffer, "%s", password) != 1) {
            send(client_sock, "Vui long khong de trong!\n", 25, 0);
            continue;
        }

        if (check_login(username, password)) {
            is_logged_in = 1;
            char *success = "\nDang nhap thanh cong!\n";
            send(client_sock, success, strlen(success), 0);
        } else {
            char *err = "\nSai tai khoan hoac mat khau! Vui long thu lai.\n\n";
            send(client_sock, err, strlen(err), 0);
        }
    }

    while (1) {
        char *cmd_prompt = "telnet> ";
        send(client_sock, cmd_prompt, strlen(cmd_prompt), 0);

        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) break;

        buffer[strcspn(buffer, "\r\n")] = 0;
        
        if (strlen(buffer) == 0) continue;

        if (strcmp(buffer, "exit") == 0) break;

        char out_filename[64];
        sprintf(out_filename, "out_%d.txt", client_sock);

        char system_cmd[BUFFER_SIZE + 100];
        sprintf(system_cmd, "%s > %s 2>&1", buffer, out_filename);

        system(system_cmd);

        FILE *fp = fopen(out_filename, "r");
        if (fp) {
            char file_buf[BUFFER_SIZE];
            size_t bytes_read;
            int has_output = 0;
            
            while ((bytes_read = fread(file_buf, 1, BUFFER_SIZE, fp)) > 0) {
                send(client_sock, file_buf, bytes_read, 0);
                has_output = 1;
            }
            fclose(fp);
            
            if (!has_output) {
                send(client_sock, "\n", 1, 0);
            }
        }
        
        remove(out_filename);
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
        printf("Telnet Server dang chay tren cong %d...\n", PORT);
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