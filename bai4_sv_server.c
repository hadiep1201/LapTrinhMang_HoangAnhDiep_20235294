#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

typedef struct {
    char mssv[20];
    char ho_ten[100];
    char ngay_sinh[20];
    float diem_tb;
} SinhVien;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Sử dụng: ./sv_server <Cổng> <Tên file log>\n");
        return 1;
    }

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(argv[1]));

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    listen(listener, 5);
    printf("Server đang đợi kết nối ở cổng %s...\n", argv[1]);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        
        int client = accept(listener, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client < 0) continue;

        // Lấy IP Client
        char *client_ip = inet_ntoa(client_addr.sin_addr);

        // Nhận struct dữ liệu
        SinhVien sv;
        int n = recv(client, &sv, sizeof(sv), 0);
        
        if (n > 0) {
            // Lấy thời gian hiện tại
            time_t t = time(NULL);
            struct tm *tm_info = localtime(&t);
            char time_str[26];
            strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);

            // In ra màn hình
            printf("%s %s %s %s %s %.2f\n", client_ip, time_str, sv.mssv, sv.ho_ten, sv.ngay_sinh, sv.diem_tb);

            // Ghi vào file log (chế độ 'a' - append)
            FILE *f = fopen(argv[2], "a");
            if (f != NULL) {
                fprintf(f, "%s %s %s %s %s %.2f\n", client_ip, time_str, sv.mssv, sv.ho_ten, sv.ngay_sinh, sv.diem_tb);
                fclose(f);
            }
        }
        close(client);
    }

    close(listener);
    return 0;
}