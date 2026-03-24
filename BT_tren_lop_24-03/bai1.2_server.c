#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#define PATTERN "0123456789"
#define PAT_LEN 10

int main(int argc, char **argv) {
    if (argc != 2) return printf("Sử dụng: %s <Port>\n", argv[0]), 1;

    // 1. Tạo socket và kiểm tra lỗi
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("Không tạo được socket");
        return 1;
    }

    struct sockaddr_in addr = {AF_INET, htons(atoi(argv[1])), INADDR_ANY};
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed (Cổng có thể đang bị chiếm)");
        return 1;
    }

    listen(s, 5);
    printf("Server đang đợi kết nối tại cổng %s...\n", argv[1]);

    struct sockaddr_in caddr;
    socklen_t clen = sizeof(caddr);
    int c = accept(s, (struct sockaddr*)&caddr, &clen);
    printf("Đã có client kết nối từ: %s\n", inet_ntoa(caddr.sin_addr));

    // 2. Nhận dữ liệu streaming và xử lý xâu bị cắt
    char buf[1024], leftover[PAT_LEN] = "";
    int total_count = 0;

    while (1) {
        int n = recv(c, buf, sizeof(buf) - 1, 0);
        if (n <= 0) break;
        buf[n] = '\0';
        char *temp = malloc(strlen(leftover) + n + 1);
        sprintf(temp, "%s%s", leftover, buf);
        char *ptr = temp;
        while ((ptr = strstr(ptr, PATTERN)) != NULL) {
            total_count++;
            ptr += PAT_LEN;
        }
        int t_len = strlen(temp);
        if (t_len >= PAT_LEN - 1) {
            strcpy(leftover, temp + t_len - (PAT_LEN - 1));
        } else {
            strcpy(leftover, temp);
        }

        printf("Tổng số lần xuất hiện '%s': %d\n", PATTERN, total_count);
        free(temp);
    }

    // 3. Đóng kết nối
    printf("\nKết nối đã đóng. Tổng cộng: %d lần.\n", total_count);
    close(c); close(s);
    return 0;
}
