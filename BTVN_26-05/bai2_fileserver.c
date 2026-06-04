#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <dirent.h>

#define PORT 8080
#define BUFFER_SIZE 8192

// Hàm xác định loại file trả về cho trình duyệt
const char *get_mime_type(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return "application/octet-stream";
    if (strcmp(dot, ".html") == 0) return "text/html";
    if (strcmp(dot, ".txt") == 0 || strcmp(dot, ".c") == 0) return "text/plain";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(dot, ".png") == 0) return "image/png";
    if (strcmp(dot, ".mp3") == 0) return "audio/mpeg";
    if (strcmp(dot, ".mp4") == 0) return "video/mp4";
    return "application/octet-stream";
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) return;
    buffer[bytes_read] = '\0';

    char method[16], uri[2048];
    sscanf(buffer, "%s %s", method, uri);

    // Chống hack thư mục (Directory Traversal)
    if (strstr(uri, "..")) {
        char *err = "HTTP/1.1 403 Forbidden\r\n\r\n403 Forbidden";
        send(client_socket, err, strlen(err), 0);
        return;
    }

    // Tăng kích thước mảng lên 4096 và dùng snprintf để tránh Warning
    char filepath[4096];
    snprintf(filepath, sizeof(filepath), ".%s", uri); 
    if (strcmp(filepath, "./") == 0) strcpy(filepath, ".");

    struct stat st;
    if (stat(filepath, &st) == -1) {
        char *err = "HTTP/1.1 404 Not Found\r\n\r\n404 File Not Found";
        send(client_socket, err, strlen(err), 0);
        return;
    }

    if (S_ISDIR(st.st_mode)) { // Nếu là Thư mục
        DIR *dir = opendir(filepath);
        if (!dir) return;

        char response_header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n"
                                 "<html><body><h2>Danh sach: %s</h2><ul>";
        char buf[BUFFER_SIZE];
        snprintf(buf, sizeof(buf), response_header, uri);
        send(client_socket, buf, strlen(buf), 0);

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0) continue;

            char linkpath[4096];
            if (strcmp(uri, "/") == 0) 
                snprintf(linkpath, sizeof(linkpath), "/%s", entry->d_name);
            else 
                snprintf(linkpath, sizeof(linkpath), "%s/%s", uri, entry->d_name);

            char real_path[4096];
            snprintf(real_path, sizeof(real_path), "%s/%s", filepath, entry->d_name);
            
            struct stat entry_stat;
            stat(real_path, &entry_stat);

            if (S_ISDIR(entry_stat.st_mode)) {
                snprintf(buf, sizeof(buf), "<li><b><a href=\"%s\">%s/</a></b></li>", linkpath, entry->d_name);
            } else {
                snprintf(buf, sizeof(buf), "<li><i><a href=\"%s\">%s</a></i></li>", linkpath, entry->d_name);
            }
            send(client_socket, buf, strlen(buf), 0);
        }
        closedir(dir);
        send(client_socket, "</ul></body></html>", 19, 0);

    } else { // Nếu là File
        FILE *file = fopen(filepath, "rb");
        if (!file) return;

        char header[512];
        snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", 
                get_mime_type(filepath), st.st_size);
        send(client_socket, header, strlen(header), 0);

        char file_buf[BUFFER_SIZE];
        size_t read_bytes;
        while ((read_bytes = fread(file_buf, 1, sizeof(file_buf), file)) > 0) {
            send(client_socket, file_buf, read_bytes, 0);
        }
        fclose(file);
    }
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1; setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 10);

    printf("Bai 2 - File Server dang chay tai: http://localhost:%d/\n", PORT);
    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        handle_client(client_socket);
        close(client_socket);
    }
    return 0;
}