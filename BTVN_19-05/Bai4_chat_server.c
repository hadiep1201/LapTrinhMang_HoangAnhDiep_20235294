#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 9000
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

// Cấu trúc lưu trữ thông tin của 1 Client
typedef struct {
    int socket;
    char id[50];
    char name[50];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Hàm xóa client khỏi danh sách khi ngắt kết nối
void remove_client(int socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket == socket) {
            // Dịch các phần tử phía sau lên để lấp chỗ trống
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Hàm gửi tin nhắn cho TẤT CẢ client khác (trừ người gửi)
void broadcast_message(char *message, int sender_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket != sender_socket) {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Hàm xử lý riêng cho từng client (Chạy trong 1 luồng riêng biệt)
void* handle_client(void* arg) {
    int client_sock = *(int*)arg;
    free(arg); // Giải phóng con trỏ

    char buffer[BUFFER_SIZE];
    char id[50], name[50];

    // ==========================================
    // BƯỚC 1: XÁC THỰC THEO ĐÚNG CÚ PHÁP
    // ==========================================
    while (1) {
        char *prompt = "Vui long dang nhap theo cu phap: client_id: client_name\n";
        send(client_sock, prompt, strlen(prompt), 0);

        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            close(client_sock);
            return NULL; // Client ngắt kết nối trước khi đăng nhập
        }

        // Xóa ký tự xuống dòng ở cuối
        buffer[strcspn(buffer, "\r\n")] = 0;

        // Dùng sscanf để tách chuỗi. %[^:] lấy tất cả trước dấu :, %s lấy chuỗi viết liền phía sau
        if (sscanf(buffer, "%[^:]: %s", id, name) == 2) {
            // Đăng nhập thành công, lưu vào mảng toàn cục
            pthread_mutex_lock(&clients_mutex);
            if (client_count < MAX_CLIENTS) {
                clients[client_count].socket = client_sock;
                strcpy(clients[client_count].id, id);
                strcpy(clients[client_count].name, name);
                client_count++;
                pthread_mutex_unlock(&clients_mutex);

                char *success = "Dang nhap thanh cong! Ban co the bat dau chat.\n";
                send(client_sock, success, strlen(success), 0);
                break; // Thoát vòng lặp đăng nhập
            } else {
                pthread_mutex_unlock(&clients_mutex);
                char *full = "Server da day!\n";
                send(client_sock, full, strlen(full), 0);
                close(client_sock);
                return NULL;
            }
        } else {
            char *err = "Sai cu phap!\n";
            send(client_sock, err, strlen(err), 0);
        }
    }

    // ==========================================
    // BƯỚC 2: VÒNG LẶP NHẬN VÀ CHUYỂN TIẾP TIN NHẮN
    // ==========================================
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            break; // Client ngắt kết nối
        }

        buffer[strcspn(buffer, "\r\n")] = 0; // Xóa ký tự xuống dòng

        if (strlen(buffer) > 0) {
            // Lấy thời gian hiện tại
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            char time_str[64];
            // Format: "2023/05/06 11:00:00PM"
            strftime(time_str, sizeof(time_str), "%Y/%m/%d %I:%M:%S%p", tm);

            // Ghép chuỗi theo format đề bài: "Thời_gian id: nội_dung"
            char send_buf[BUFFER_SIZE + 150];
            snprintf(send_buf, sizeof(send_buf), "%s %s: %s\n", time_str, id, buffer);

            // Gửi cho tất cả mọi người khác
            broadcast_message(send_buf, client_sock);
        }
    }

    // ==========================================
    // BƯỚC 3: DỌN DẸP KHI CLIENT THOÁT
    // ==========================================
    remove_client(client_sock);
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
        printf("Group Chat Server dang chay tren cong %d...\n", PORT);
    } else {
        exit(1);
    }

    while (1) {
        addr_size = sizeof(client_addr);
        int client_sock = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        if (client_sock < 0) continue;

        printf("Co client moi ket noi: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Cấp phát động bộ nhớ cho biến socket để truyền vào luồng một cách an toàn
        new_socket = malloc(sizeof(int));
        *new_socket = client_sock;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)new_socket) != 0) {
            perror("Khong the tao luong");
            free(new_socket);
        } else {
            pthread_detach(thread_id); // Tách luồng để tự động dọn rác khi kết thúc
        }
    }

    close(server_socket);
    return 0;
}