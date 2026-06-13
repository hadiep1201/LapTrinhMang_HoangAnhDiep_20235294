#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket close

// ==========================================
// THÔNG TIN CỦA BẠN (Đã điền sẵn chuẩn 100%)
// ==========================================
#define FTP_SERVER "lebavui.io.vn"
#define FTP_PORT 21
#define MSSV "20235294"        
#define NGAY_SINH "12"         

char USERNAME[50];
char PASSWORD[50];

void send_cmd(SOCKET sock, const char* cmd) {
    send(sock, cmd, strlen(cmd), 0);
    printf("C->S: %s", cmd);
}

int recv_res(SOCKET sock, char* buffer, int size) {
    memset(buffer, 0, size);
    int bytes = recv(sock, buffer, size - 1, 0);
    if (bytes > 0) printf("S->C: %s", buffer);
    return bytes;
}

SOCKET enter_passive_mode(SOCKET control_sock) {
    char buffer[1024];
    send_cmd(control_sock, "PASV\r\n");
    recv_res(control_sock, buffer, sizeof(buffer));

    int h1, h2, h3, h4, p1, p2;
    char *start = strchr(buffer, '(');
    if (start != NULL) {
        sscanf(start, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);
    } else return INVALID_SOCKET;

    char data_ip[32];
    sprintf(data_ip, "%d.%d.%d.%d", h1, h2, h3, h4);
    int data_port = p1 * 256 + p2;
    
    SOCKET data_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data_port);
    server_addr.sin_addr.s_addr = inet_addr(data_ip);

    if (connect(data_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) return INVALID_SOCKET;
    return data_sock;
}

void reverse_string(char *str, int length) {
    for (int i = 0; i < length / 2; i++) {
        char temp = str[i];
        str[i] = str[length - i - 1];
        str[length - i - 1] = temp;
    }
}

int main() {
    sprintf(USERNAME, "user_%s", MSSV);
    const char *last_4_mssv = MSSV + strlen(MSSV) - 4;
    sprintf(PASSWORD, "%s%s", last_4_mssv, NGAY_SINH);

    SOCKET control_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    char buffer[4096];
    char cmd[256];

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(FTP_PORT);
    struct hostent *host = gethostbyname(FTP_SERVER);
    memcpy(&server_addr.sin_addr, host->h_addr_list[0], host->h_length);

    printf("\n=== DANG KET NOI TOI %s ===\n", FTP_SERVER);
    if (connect(control_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) return 1;
    recv_res(control_sock, buffer, sizeof(buffer));

    sprintf(cmd, "USER %s\r\n", USERNAME);
    send_cmd(control_sock, cmd); recv_res(control_sock, buffer, sizeof(buffer));

    sprintf(cmd, "PASS %s\r\n", PASSWORD);
    send_cmd(control_sock, cmd); recv_res(control_sock, buffer, sizeof(buffer));

    printf("\n=== TIM FILE QUESTION ===\n");
    SOCKET data_sock = enter_passive_mode(control_sock);
    send_cmd(control_sock, "LIST\r\n");
    recv_res(control_sock, buffer, sizeof(buffer));

    char file_list[8192] = {0};
    int bytes;
    while ((bytes = recv(data_sock, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[bytes] = '\0'; strcat(file_list, buffer);
    }
    closesocket(data_sock); recv_res(control_sock, buffer, sizeof(buffer));

    char question_file[100] = {0}, random_code[50] = {0};
    char *start_ptr = strstr(file_list, "question_");
    if (start_ptr) {
        char *end_ptr = strstr(start_ptr, ".txt");
        if (end_ptr) {
            strncpy(question_file, start_ptr, end_ptr - start_ptr + 4);
            strncpy(random_code, start_ptr + 9, end_ptr - (start_ptr + 9));
            printf("\n-> Da tim thay: %s (Ma: %s)\n", question_file, random_code);
        }
    }

    if(strlen(question_file) == 0) {
        printf("-> LOI: Khong tim thay file. Ban da tao file question tren server chua?\n");
        return 1;
    }

    printf("\n=== TAI FILE VA XU LY ===\n");
    send_cmd(control_sock, "TYPE I\r\n"); recv_res(control_sock, buffer, sizeof(buffer));

    data_sock = enter_passive_mode(control_sock);
    sprintf(cmd, "RETR %s\r\n", question_file);
    send_cmd(control_sock, cmd); recv_res(control_sock, buffer, sizeof(buffer));

    FILE *fq = fopen(question_file, "wb"); // Mở file question trên ổ cứng
    char file_content[1024] = {0}; int content_len = 0;
    while ((bytes = recv(data_sock, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes, fq);      // Ghi dữ liệu xuống ổ cứng
        memcpy(file_content + content_len, buffer, bytes); 
        content_len += bytes;
    }
    fclose(fq); // Đóng lưu file
    
    closesocket(data_sock); recv_res(control_sock, buffer, sizeof(buffer));

    printf("\n=========================================================\n");
    printf("NOI DUNG FILE TAI VE:\n");
    if (file_content[content_len - 1] == '\n') { file_content[content_len - 1] = '\0'; content_len--; }
    printf("%s\n", file_content);
    printf("=========================================================\n\n");

    char answer_file[100];
    sprintf(answer_file, "answer_%s.txt", random_code);
    reverse_string(file_content, content_len); 
    
    FILE *fa = fopen(answer_file, "wb");
    fwrite(file_content, 1, content_len, fa); fclose(fa);
    printf("-> Da tao file %s voi noi dung dao nguoc.\n", answer_file);

    printf("\n=== UPLOAD FILE ANSWER ===\n");
    data_sock = enter_passive_mode(control_sock);
    sprintf(cmd, "STOR %s\r\n", answer_file);
    send_cmd(control_sock, cmd); recv_res(control_sock, buffer, sizeof(buffer));

    fa = fopen(answer_file, "rb");
    while ((bytes = fread(buffer, 1, sizeof(buffer), fa)) > 0) send(data_sock, buffer, bytes, 0);
    fclose(fa); closesocket(data_sock); recv_res(control_sock, buffer, sizeof(buffer));

    printf("\n=== HOAN THANH ===\n");
    send_cmd(control_sock, "QUIT\r\n"); recv_res(control_sock, buffer, sizeof(buffer));
    closesocket(control_sock);
    return 0;
}