#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 8192

// Hàm giải mã URL (Ví dụ: %2B -> '+')
void url_decode(char *src, char *dest) {
    char *p = src;
    char code[3] = {0};
    while (*p) {
        if (*p == '%') {
            memcpy(code, ++p, 2);
            *dest++ = (char)strtoul(code, NULL, 16);
            p += 2;
        } else if (*p == '+') {
            *dest++ = ' '; // Trong HTTP, dấu + không mã hóa thường mang ý nghĩa là dấu cách
            p++;
        } else {
            *dest++ = *p++;
        }
    }
    *dest = '\0';
}

// Hàm lấy tham số từ chuỗi dữ liệu (query string hoặc body)
void get_param(const char *data, const char *param_name, char *result) {
    char search_str[64];
    sprintf(search_str, "%s=", param_name);
    char *pos = strstr(data, search_str);
    if (pos) {
        pos += strlen(search_str);
        char *end = strpbrk(pos, "& \r\n");
        if (end) {
            strncpy(result, pos, end - pos);
            result[end - pos] = '\0';
        } else {
            strcpy(result, pos);
        }
    } else {
        result[0] = '\0';
    }
}

// Hàm xử lý phép tính
void handle_calc(int client_socket, const char *params) {
    char a_str[32], b_str[32], op_str[16], decoded_op[16];
    
    get_param(params, "a", a_str);
    get_param(params, "b", b_str);
    get_param(params, "op", op_str);
    
    // Giải mã toán tử
    url_decode(op_str, decoded_op);

    double a = atof(a_str);
    double b = atof(b_str);
    double result = 0;
    char result_msg[256];

    // Chú ý: decode_op có thể là "+" hoặc " " (nếu trình duyệt gửi trực tiếp dấu + không mã hóa)
    if (strcmp(decoded_op, "+") == 0 || strcmp(decoded_op, " ") == 0) {
        result = a + b;
        sprintf(result_msg, "%.2lf + %.2lf = %.2lf", a, b, result);
    } else if (strcmp(decoded_op, "-") == 0) {
        result = a - b;
        sprintf(result_msg, "%.2lf - %.2lf = %.2lf", a, b, result);
    } else if (strcmp(decoded_op, "*") == 0) {
        result = a * b;
        sprintf(result_msg, "%.2lf * %.2lf = %.2lf", a, b, result);
    } else if (strcmp(decoded_op, "/") == 0) {
        if (b == 0) strcpy(result_msg, "Loi: Khong the chia cho 0!");
        else {
            result = a / b;
            sprintf(result_msg, "%.2lf / %.2lf = %.2lf", a, b, result);
        }
    } else {
        strcpy(result_msg, "Toan tu khong hop le!");
    }

    char response[1024];
    sprintf(response, 
        "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n"
        "<html><body><h2>Ket qua phep tinh:</h2><h3>%s</h3>"
        "<a href='/calc'>Quay lai</a></body></html>", result_msg);
    send(client_socket, response, strlen(response), 0);
}

// Xử lý request
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) return;
    buffer[bytes_read] = '\0';

    char method[16], uri[2048];
    sscanf(buffer, "%s %s", method, uri);

    if (strncmp(uri, "/calc", 5) == 0) {
        if (strcmp(method, "GET") == 0) {
            char *query = strchr(uri, '?');
            if (query) { // Có chứa dấu ? nghĩa là có tham số truyền vào
                handle_calc(client_socket, query + 1);
            } else { // Không có tham số, hiển thị Form nhập liệu
                char *form = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n"
                             "<html><body><h2>May tinh HTTP</h2>"
                             "<form action='/calc' method='GET'>"
                             "<b>Method GET:</b> <input type='text' name='a' size='5'> "
                             "<select name='op'><option value='+'>+</option><option value='-'>-</option><option value='*'>*</option><option value='/'>/</option></select> "
                             "<input type='text' name='b' size='5'> <input type='submit' value='Tinh'></form>"
                             "<form action='/calc' method='POST'>"
                             "<b>Method POST:</b> <input type='text' name='a' size='5'> "
                             "<select name='op'><option value='+'>+</option><option value='-'>-</option><option value='*'>*</option><option value='/'>/</option></select> "
                             "<input type='text' name='b' size='5'> <input type='submit' value='Tinh'></form>"
                             "</body></html>";
                send(client_socket, form, strlen(form), 0);
            }
        } else if (strcmp(method, "POST") == 0) {
            char *body = strstr(buffer, "\r\n\r\n");
            if (body) handle_calc(client_socket, body + 4);
        }
    } else {
        // Tự động chuyển hướng về trang /calc nếu vào trang chủ
        char *redirect = "HTTP/1.1 302 Found\r\nLocation: /calc\r\n\r\n";
        send(client_socket, redirect, strlen(redirect), 0);
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

    printf("Bai 1 - Tinh toan dang chay tai: http://localhost:%d/calc\n", PORT);
    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        handle_client(client_socket);
        close(client_socket);
    }
    return 0;
}