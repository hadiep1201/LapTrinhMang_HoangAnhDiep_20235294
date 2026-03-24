#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char **argv){
    if(argc!=3){
        printf("Sử dụng: %s <IP> <Port>\n", argv[0]);
        return 1;
    }
    //1.Tạo socket
    int s = socket(AF_INET, SOCK_STREAM,0);
    if(s<0){
        perror("Không tạo được socket");
        return 1;
    }
    struct sockaddr_in addr = {AF_INET, htons(atoi(argv[2])), {inet_addr(argv[1])}};
    printf("Đang kết nối tới server: %s:%s...\n", argv[1], argv[2]);
    if(connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        perror("Kết nối thất bại");
        return 1;
    }
    printf("Đã kết nối thành công! Nhập dữ liệu vào:\n");
    //2. Gửi dữ liệu liên tục từ bàn phím
    char buf[1024];
    while (fgets(buf, sizeof(buf), stdin)) {
        buf[strcspn(buf, "\n")] = 0; 
        if (strlen(buf) > 0) {
            send(s, buf, strlen(buf), 0);
        }
    }

    close(s);
    return 0;
}
