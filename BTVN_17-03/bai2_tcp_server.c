#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
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
    addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    addr.sin_port = htons(port);

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5); 
    printf("Server đang đợi ở cổng %d...\n", port);

    // 3. Đọc file câu chào và gửi cho client
    while (1) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        int client = accept(listener, (struct sockaddr *)&client_addr, &client_len);
        printf("Có client mới kết nối!\n");

        // 4. Gửi câu chào
        FILE *f_chao = fopen(file_chao, "r");
        if (f_chao != NULL) {
            char buf_send[1024];
            int n = fread(buf_send, 1, sizeof(buf_send), f_chao);
            send(client, buf_send, n, 0);
            fclose(f_chao);
        }

        // 5. Nhận dữ liệu và GHI FILE NGAY LẬP TỨC
        char buf_recv[1024];
        int ret;
        
        FILE *f_out = fopen(file_luu, "a");
        
        while ((ret = recv(client, buf_recv, sizeof(buf_recv) - 1, 0)) > 0) {
            buf_recv[ret] = '\0';
            // In ra màn hình console
            printf("Đã nhận: %s", buf_recv);

            // Ghi vào file và dùng fflush để đẩy dữ liệu xuống ổ cứng ngay
            if (f_out != NULL) {
                fprintf(f_out, "%s", buf_recv);
                fflush(f_out); 
            }
        }

        if (f_out != NULL) fclose(f_out);
        close(client);
        printf("Client đã ngắt kết nối. Đang đợi khách tiếp theo...\n");
    }

    close(listener);
    return 0; 
}