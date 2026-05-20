#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 9000
#define BUFFER_SIZE 1024

int waiting_client = -1;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

struct Pair {
    int client1;
    int client2;
};

void* handle_pair(void* arg) {
    struct Pair* pair = (struct Pair*)arg;
    int c1 = pair->client1;
    int c2 = pair->client2;
    free(pair); 

    char *msg = "Da ghep cap thanh cong! Bat dau chat...\n";
    send(c1, msg, strlen(msg), 0);
    send(c2, msg, strlen(msg), 0);

    fd_set readfds;
    int max_fd = (c1 > c2) ? c1 : c2;
    char buffer[BUFFER_SIZE];

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(c1, &readfds);
        FD_SET(c2, &readfds);

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) break;

        if (FD_ISSET(c1, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes = recv(c1, buffer, BUFFER_SIZE, 0);
            if (bytes <= 0) break; 
            send(c2, buffer, bytes, 0); 
        }

        if (FD_ISSET(c2, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes = recv(c2, buffer, BUFFER_SIZE, 0);
            if (bytes <= 0) break; 
            send(c1, buffer, bytes, 0); 
        }
    }

    printf("Mot cap client da ngat ket noi.\n");
    close(c1);
    close(c2);
    return NULL;
}

int main() {
    int server_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) exit(1);

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) exit(1);

    if (listen(server_socket, 10) == 0) {
        printf("Chat Server dang chay tren cong %d...\n", PORT);
    } else {
        exit(1);
    }

    while (1) {
        addr_size = sizeof(client_addr);
        new_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        if (new_socket < 0) continue;

        printf("Co client moi ket noi: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_mutex_lock(&queue_mutex); 
        
        if (waiting_client == -1) {
            waiting_client = new_socket;
            char *wait_msg = "Dang cho nguoi ghep cap...\n";
            send(new_socket, wait_msg, strlen(wait_msg), 0);
            pthread_mutex_unlock(&queue_mutex); 
        } else {
            struct Pair* p = malloc(sizeof(struct Pair));
            p->client1 = waiting_client;
            p->client2 = new_socket;
            
            waiting_client = -1; 
            pthread_mutex_unlock(&queue_mutex); 

            pthread_t thread_id;
            pthread_create(&thread_id, NULL, handle_pair, (void*)p);
            pthread_detach(thread_id); 
        }
    }

    close(server_socket);
    return 0;
}