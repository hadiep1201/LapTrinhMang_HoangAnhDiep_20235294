#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    // 1. Kiểm tra tham số dòng lệnh (argc < 3 vì gồm: tên_file, ip, port)
    if (argc < 3) {
        printf("Sử dụng: ./tcp_client <Địa chỉ IP> <Cổng>\n");
        return 1;
    }

    // 2. Lấy tham số từ dòng lệnh
    char *ip = argv[1];
    int port = atoi(argv[2]); // Chuyển chuỗi cổng sang số nguyên

    // 3. Tạo socket (AF_INET: IPv4, SOCK_STREAM: TCP)
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // 4. Khai báo địa chỉ của server để kết nối
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip); // Chuyển chuỗi IP sang dạng số
    addr.sin_port = htons(port);          // Chuyển cổng sang định dạng mạng (Big-endian)

    // 5. Kết nối đến server
    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Kết nối thất bại");
        return 1;
    }
    printf("Đã kết nối thành công đến %s:%d\n", ip, port);
    char welcome_msg[1024];
    int n = recv(client, welcome_msg, sizeof(welcome_msg) - 1, 0);
    if (n > 0) {
        welcome_msg[n] = '\0'; // Kết thúc chuỗi
        printf("Tin nhắn từ Server: %s\n", welcome_msg);
    }

    // 6. Nhận dữ liệu từ bàn phím và gửi đến server
    char buf[1024];
    while (1) {
        printf("Nhập tin nhắn: ");
        fgets(buf, sizeof(buf), stdin); // Đọc chuỗi từ bàn phím
        
        send(client, buf, strlen(buf), 0); // Gửi sang server
        if (strncmp(buf, "exit", 4) == 0) break; // Thoát nếu gõ exit
    }

    close(client);
    return 0;
}