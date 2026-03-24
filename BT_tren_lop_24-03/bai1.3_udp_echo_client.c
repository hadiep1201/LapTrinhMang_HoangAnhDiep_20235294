#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 9000
#define BUF_SIZE 2048

int main() {
    int sockfd;
    char buffer[BUF_SIZE], message[BUF_SIZE];
    struct sockaddr_in servaddr;
    socklen_t len = sizeof(servaddr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket failed");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    while (1) {
        printf("\nNhập tin nhắn: ");
        if (fgets(message, BUF_SIZE, stdin) == NULL) break;
        message[strcspn(message, "\n")] = 0;
        sendto(sockfd, message, strlen(message), 0, (const struct sockaddr *)&servaddr, len);
        if (strcmp(message, "exit") == 0) break;
        printf("Đã gửi đến Server: %s\n", message);

        int n = recvfrom(sockfd, buffer, BUF_SIZE - 1, 0, (struct sockaddr *)&servaddr, &len);
        if (n > 0) {
            buffer[n] = '\0';
            printf("Server phản hồi: %s\n", buffer);
        }
    }

    close(sockfd);
    printf("Đã đóng kết nối.\n");
    return 0;
}