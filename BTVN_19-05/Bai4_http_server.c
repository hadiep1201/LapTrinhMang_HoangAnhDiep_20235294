#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define NUM_THREADS 8     // Số lượng luồng tạo sẵn (Pre-threads)
#define QUEUE_SIZE 100    // Kích thước hàng đợi kết nối

int client_queue[QUEUE_SIZE];
int queue_head = 0;
int queue_tail = 0;
int queue_count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_not_full = PTHREAD_COND_INITIALIZER;

void* worker_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        
        while (queue_count == 0) {
            pthread_cond_wait(&cond_not_empty, &mutex);
        }
        
        int client = client_queue[queue_head];
        queue_head = (queue_head + 1) % QUEUE_SIZE;
        queue_count--;
        
        pthread_cond_signal(&cond_not_full);
        pthread_mutex_unlock(&mutex);

        // --- Bắt đầu phần xử lý HTTP giống trên Slide ---
        char buf[2048];
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        
        if (ret > 0) {
            buf[ret] = 0;
            puts(buf); // In request của trình duyệt ra màn hình console

            char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xin chao cac ban</h1></body></html>";
            send(client, msg, strlen(msg), 0);
        }
        
        close(client);
        // --- Kết thúc xử lý, luồng quay lại vòng lặp chờ việc mới ---
    }
    return NULL;
}

int main() {
    int listener;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) exit(1);

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) exit(1);

    if (listen(listener, 10) == 0) {
        printf("HTTP Server dang chay tren cong %d...\n", PORT);
    } else {
        exit(1);
    }

    // Khởi tạo trước nhóm luồng (Pre-threading)
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }

    // Luồng chính (Main thread) chỉ làm nhiệm vụ Nhận kết nối và đẩy vào Hàng đợi
    while (1) {
        addr_size = sizeof(client_addr);
        int client = accept(listener, (struct sockaddr*)&client_addr, &addr_size);
        if (client < 0) continue;

        printf("New client connected: %d\n", client);

        pthread_mutex_lock(&mutex);
        
        while (queue_count == QUEUE_SIZE) {
            pthread_cond_wait(&cond_not_full, &mutex);
        }
        
        client_queue[queue_tail] = client;
        queue_tail = (queue_tail + 1) % QUEUE_SIZE;
        queue_count++;
        
        pthread_cond_signal(&cond_not_empty); // Gọi 1 luồng rảnh rỗi dậy làm việc
        pthread_mutex_unlock(&mutex);
    }

    close(listener);
    return 0;
}