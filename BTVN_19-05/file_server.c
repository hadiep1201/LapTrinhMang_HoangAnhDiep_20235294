#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_DIR "./server_files" 

void sigchld_handler(int s) {
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    DIR *d;
    struct dirent *dir;
    int file_count = 0;
    char file_list[8192] = ""; 
    
    d = opendir(SERVER_DIR);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                file_count++;
                strcat(file_list, dir->d_name);
                strcat(file_list, "\r\n");
            }
        }
        closedir(d);
    }

    if (file_count == 0) {
        char *error_msg = "ERROR No files to download\r\n";
        send(client_socket, error_msg, strlen(error_msg), 0);
        close(client_socket);
        return; 
    } else {
        char header[256];
        sprintf(header, "OK %d\r\n", file_count);
        send(client_socket, header, strlen(header), 0); 
        send(client_socket, file_list, strlen(file_list), 0);
        send(client_socket, "\r\n", 2, 0); 
    }

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) break;

        buffer[strcspn(buffer, "\r\n")] = 0;
        
        if (strchr(buffer, '/') != NULL) {
            char *err = "ERROR Invalid filename\r\n";
            send(client_socket, err, strlen(err), 0);
            continue; 
        }

        char filepath[2048];
        snprintf(filepath, sizeof(filepath), "%s/%s", SERVER_DIR, buffer);

        struct stat file_stat;
        if (stat(filepath, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
            char file_header[256];
            sprintf(file_header, "OK %ld\r\n", file_stat.st_size);
            send(client_socket, file_header, strlen(file_header), 0);

            FILE *fp = fopen(filepath, "rb");
            if (fp) {
                int bytes_read;
                char file_buffer[BUFFER_SIZE];
                while ((bytes_read = fread(file_buffer, 1, BUFFER_SIZE, fp)) > 0) {
                    send(client_socket, file_buffer, bytes_read, 0);
                }
                fclose(fp);
            }
            break; 
        } else {
            char *err = "ERROR File not found\r\n";
            send(client_socket, err, strlen(err), 0);
        }
    }

    close(client_socket);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) exit(1);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) exit(1);

    if (listen(server_socket, 10) == 0) {
        printf("Server dang lang nghe tren cong %d...\n", PORT);
    } else {
        exit(1);
    }

    while (1) {
        addr_size = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        
        if (client_socket < 0) continue;

        pid_t pid = fork();

        if (pid == 0) {
            close(server_socket); 
            handle_client(client_socket);
            exit(0); 
        } else if (pid > 0) {
            close(client_socket); 
        }
    }

    close(server_socket);
    return 0;
}