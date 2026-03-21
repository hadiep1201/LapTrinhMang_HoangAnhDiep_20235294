#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Sử dụng: ./sv_client <IP> <Cổng>\n");
        return 1;
    }

    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Kết nối thất bại");
        return 1;
    }

    char mssv[20], ho_ten[100], ngay_sinh[20], diem_tb[10];
    char send_buf[256];

    // Nhập dữ liệu
    printf("Nhập MSSV: "); fgets(mssv, sizeof(mssv), stdin); mssv[strcspn(mssv, "\n")] = 0;
    printf("Nhập Họ tên: "); fgets(ho_ten, sizeof(ho_ten), stdin); ho_ten[strcspn(ho_ten, "\n")] = 0;
    printf("Nhập Ngày sinh (YYYY-MM-DD): "); fgets(ngay_sinh, sizeof(ngay_sinh), stdin); ngay_sinh[strcspn(ngay_sinh, "\n")] = 0;
    printf("Nhập Điểm TB: "); fgets(diem_tb, sizeof(diem_tb), stdin); diem_tb[strcspn(diem_tb, "\n")] = 0;

    // Đóng gói dữ liệu thành chuỗi cách nhau bởi dấu cách
    sprintf(send_buf, "%s %s %s %s", mssv, ho_ten, ngay_sinh, diem_tb);

    // Gửi sang server
    send(client, send_buf, strlen(send_buf), 0);

    close(client);
    return 0;
}