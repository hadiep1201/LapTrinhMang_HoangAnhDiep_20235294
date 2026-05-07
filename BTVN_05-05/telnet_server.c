#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 8080

void sigchld_handler(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int check_login(char *username, char *password) {
    FILE *f = fopen("taikhoan.txt", "r"); 
    if (!f) {
        perror("Khong tim thay file taikhoan.txt");
        return 0;
    }

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

void handle_client(int client_socket) {
    char buf[256];
    char username[64];
    char password[64];
    int logged_in = 0;

    char *welcome = "Welcome to Telnet Server!\n";
    send(client_socket, welcome, strlen(welcome), 0);

    while (!logged_in) {
        send(client_socket, "Username: ", 10, 0);
        int n = recv(client_socket, buf, sizeof(buf) - 1, 0);
        if (n <= 0) { close(client_socket); exit(0); }
        buf[n] = '\0'; buf[strcspn(buf, "\r\n")] = 0;
        strcpy(username, buf);

        send(client_socket, "Password: ", 10, 0);
        n = recv(client_socket, buf, sizeof(buf) - 1, 0);
        if (n <= 0) { close(client_socket); exit(0); }
        buf[n] = '\0'; buf[strcspn(buf, "\r\n")] = 0;
        strcpy(password, buf);

        if (check_login(username, password)) {
            logged_in = 1;
            send(client_socket, "Dang nhap thanh cong!\n", 22, 0);
        } else {
            send(client_socket, "Sai user hoac pass. Vui long dang nhap lai.\n\n", 45, 0);
        }
    }

    while (1) {
        send(client_socket, "Command: ", 9, 0);
        int n = recv(client_socket, buf, sizeof(buf) - 1, 0);
        if (n <= 0) break;
        
        buf[n] = '\0'; 
        buf[strcspn(buf, "\r\n")] = 0;

        if (strlen(buf) == 0) continue;

        if (strncmp(buf, "exit", 4) == 0) {
            printf("[-] Client (PID: %d) da thoat.\n", getpid());
            break;
        }

        char sys_cmd[512];
        char out_file[64];
        sprintf(out_file, "out_%d.txt", getpid());
        sprintf(sys_cmd, "%s > %s 2>&1", buf, out_file);
        system(sys_cmd);

        FILE *f = fopen(out_file, "r");
        if (f) {
            char out_buf[1024];
            int bytes_read;
            while ((bytes_read = fread(out_buf, 1, sizeof(out_buf) - 1, f)) > 0) {
                out_buf[bytes_read] = '\0';
                send(client_socket, out_buf, bytes_read, 0);
            }
            fclose(f);
        } else {
            send(client_socket, "Khong the doc ket qua lenh.\n", 28, 0);
        }
        
        remove(out_file);
    }

    close(client_socket);
    exit(0);
}

int main() {
    int listener_socket, client_socket;
    struct sockaddr_in server_addr;

    signal(SIGCHLD, sigchld_handler);

    listener_socket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listener_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(listener_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listener_socket, 10);

    printf("Telnet Server dang chay tai cong %d\n", PORT);

    while (1) {
        client_socket = accept(listener_socket, NULL, NULL);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        pid_t pid = fork();

        if (pid == 0) {
            close(listener_socket);
            printf("[+] Co ket noi moi (Xu ly boi PID: %d)\n", getpid());
            handle_client(client_socket);
        } 
        else if (pid > 0) {
            close(client_socket);
        } 
        else {
            perror("Fork failed");
        }
    }

    close(listener_socket);
    return 0;
}