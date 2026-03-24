#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Sử dụng: %s <Port>\n", argv[0]);
        return 1;
    }
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(atoi(argv[1])), INADDR_ANY};
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    listen(s, 5);
    printf("Server đang đợi kết nối tại cổng %s...\n", argv[1]);
    int c = accept(s, (struct sockaddr*)&client_addr, &client_len);
    printf("Đã có client kết nối từ: %s\n\n", inet_ntoa(client_addr.sin_addr));

    // 1. Nhận và in tên thư mục
    int len;
    if (recv(c, &len, sizeof(int), 0) > 0) {
        char *path = malloc(len + 1);
        recv(c, path, len, 0);
        path[len] = '\0';
        printf("%s\n", path);
        free(path);
    }

    // 2. Nhận và in danh sách file (Giải mã gói tin)
    while (recv(c, &len, sizeof(int), 0) > 0 && len > 0) {
        char *name = malloc(len + 1);
        recv(c, name, len, 0);
        name[len] = '\0';
        long size;
        recv(c, &size, sizeof(long), 0);
        printf("%s - %ld bytes\n", name, size);
        free(name);
    }

    // 3. Kết thúc
    printf("\nĐã nhận xong dữ liệu. Đóng kết nối.\n");
    close(c);
    close(s);
    return 0;
}