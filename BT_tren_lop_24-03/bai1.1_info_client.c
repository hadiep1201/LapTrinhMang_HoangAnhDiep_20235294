#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Sử dụng: %s <IP Server> <Port>\n", argv[0]);
        return 1;
    }

    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(atoi(argv[2])), {inet_addr(argv[1])}};

    printf("Đang kết nối tới server %s:%s...\n", argv[1], argv[2]);
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Kết nối thất bại");
        return 1;
    }
    printf("Đã kết nối thành công!\n");

    // 1. Gửi tên thư mục hiện tại
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    int dir_len = strlen(cwd);
    send(s, &dir_len, sizeof(int), 0);
    send(s, cwd, dir_len, 0);

    // 2. Duyệt và gửi danh sách file
    DIR *d = opendir(".");
    struct dirent *entry;
    struct stat st;
    printf("Đang đóng gói và gửi dữ liệu...\n");

    while ((entry = readdir(d)) != NULL) {
        if (stat(entry->d_name, &st) == 0 && S_ISREG(st.st_mode)) {
            int name_len = strlen(entry->d_name);
            long file_size = st.st_size;
            send(s, &name_len, sizeof(int), 0);
            send(s, entry->d_name, name_len, 0);
            send(s, &file_size, sizeof(long), 0);
        }
    }

    // 3. Gửi tín hiệu kết thúc 
    int stop = 0;
    send(s, &stop, sizeof(int), 0);

    printf("Dữ liệu đã được gửi hoàn tất.\n");
    closedir(d);
    close(s);
    return 0;
}