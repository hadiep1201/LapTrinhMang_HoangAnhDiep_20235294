#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Sử dụng: ./sv_server <cổng> <file_log>\n");
        return 1;
    }

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(argv[1]));

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);

    printf("Server sinh viên đang đợi ở cổng %s...\n", argv[1]);

    while (1) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        int client = accept(listener, (struct sockaddr *)&client_addr, &client_len);

        // 1. Lấy địa chỉ IP của client
        char *client_ip = inet_ntoa(client_addr.sin_addr);

        // 2. Lấy thời gian hiện tại
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char time_str[26];
        strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);

        // 3. Nhận dữ liệu từ client
        char buf[256];
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret > 0) {
            buf[ret] = '\0';

            // 4. In ra màn hình và ghi vào file log
            printf("%s %s %s\n", client_ip, time_str, buf);

            FILE *f = fopen(argv[2], "a");
            fprintf(f, "%s %s %s\n", client_ip, time_str, buf);
            fclose(f);
        }

        close(client);
    }

    return 0;
}