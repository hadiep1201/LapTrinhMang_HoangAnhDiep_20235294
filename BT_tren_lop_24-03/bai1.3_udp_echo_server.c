#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9000
#define BUF_SIZE 2048

int main() {
    int sockfd;
    char buffer[BUF_SIZE];
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len = sizeof(cliaddr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket failed");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    printf("Server đang chạy tại cổng %d...\n", PORT);

    while (1) {
        printf("\nĐang chờ dữ liệu...\n");
        int n = recvfrom(sockfd, buffer, BUF_SIZE - 1, 0, (struct sockaddr *)&cliaddr, &len);
            
        if (n > 0) {
            buffer[n] = '\0';

            if (strcmp(buffer, "exit") == 0) {
                printf("Nhận lệnh exit từ Client. Đang dừng Server...\n");
                break; 
            }

            printf("Nhận từ Client: %s:%d: %s\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), buffer);
            sendto(sockfd, buffer, n, 0, (struct sockaddr *)&cliaddr, len);
            printf("Đã phản hồi lại cho Client.\n");
        }
    }

    close(sockfd);
    return 0;
}