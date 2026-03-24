#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

typedef struct {
    char mssv[20];
    char ho_ten[100];
    char ngay_sinh[20];
    float diem_tb;
} SinhVien;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Sử dụng: ./sv_client <Địa chỉ IP> <Cổng>\n");
        return 1;
    }

    // 1. Tạo socket
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == -1) {
        perror("Lỗi tạo socket");
        return 1;
    }

    // 2. Thiết lập địa chỉ server
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    // 3. Kết nối
    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Kết nối thất bại");
        return 1;
    }

    // 4. Nhập dữ liệu sinh viên
    SinhVien sv;
    char temp[10];

    printf("Nhập MSSV: ");
    fgets(sv.mssv, sizeof(sv.mssv), stdin);
    sv.mssv[strcspn(sv.mssv, "\n")] = 0; // Xóa ký tự xuống dòng

    printf("Nhập Họ tên: ");
    fgets(sv.ho_ten, sizeof(sv.ho_ten), stdin);
    sv.ho_ten[strcspn(sv.ho_ten, "\n")] = 0;

    printf("Nhập Ngày sinh (YYYY-MM-DD): ");
    fgets(sv.ngay_sinh, sizeof(sv.ngay_sinh), stdin);
    sv.ngay_sinh[strcspn(sv.ngay_sinh, "\n")] = 0;

    printf("Nhập Điểm TB: ");
    fgets(temp, sizeof(temp), stdin);
    sv.diem_tb = atof(temp);

    // 5. Gửi nguyên cấu trúc struct sang server
    send(client, &sv, sizeof(sv), 0);

    printf("Đã gửi thông tin sinh viên.\n");

    close(client);
    return 0;
}