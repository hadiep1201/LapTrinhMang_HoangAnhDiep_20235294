#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    // 1. Kiểm tra tham số (Cổng, File chào, File lưu)
    if (argc < 4) {
        printf("Sử dụng: ./tcp_server <cổng> <file_chào> <file_lưu>\n");
        return 1;
    }

    int port = atoi(argv[1]);
    char *file_chao = argv[2];
    char *file_luu = argv[3];

    // 2. Tạo socket và gán vào cổng (Bind)
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // Nhận kết nối từ mọi IP
    addr.sin_port = htons(port);

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5); // Đợi kết nối (tối đa 5 hàng đợi)

    printf("Server đang đợi ở cổng %d...\n", port);

    // 3. Chấp nhận kết nối từ Client
    struct sockaddr_in client_addr;
    int client_len = sizeof(client_addr);
    int client = accept(listener, (struct sockaddr *)&client_addr, &client_len);
    printf("Có client mới kết nối!\n");

    // 4. Đọc file câu chào và gửi cho client
    FILE *f_chao = fopen(file_chao, "r");
    char buf[1024];
    int n = fread(buf, 1, sizeof(buf), f_chao);
    send(client, buf, n, 0);
    fclose(f_chao);

    // 5. Nhận dữ liệu từ client và ghi vào file khác
    FILE *f_luu = fopen(file_luu, "a"); // "a" là ghi thêm vào cuối file
    while (1) {
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) break; // Client ngắt kết nối

        buf[ret] = '\0'; // Kết thúc chuỗi
        fprintf(f_luu, "%s", buf); // Ghi vào file
        printf("Đã lưu dữ liệu từ client: %s", buf);
    }

    fclose(f_luu);
    close(client);
    close(listener);
    return 0;
}