//서버는 클라이언트의 요청을 받아 TMDB API를 호출하여 영화 데이터를 처리하고 응답을 클라이언트에 전달 
//멀티스레딩을 통해 여러 클라이언트의 요청을 처리

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "movie_manager.h"

void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[1024];

    // 클라이언트로부터 요청 받기
    read(client_socket, buffer, sizeof(buffer));
    
    // TMDB API를 사용해 영화 데이터 가져오기
    char *movie_data = get_movie_data_from_tmdb(buffer);
    
    // 클라이언트에 영화 데이터 보내기
    write(client_socket, movie_data, strlen(movie_data));

    // 클라이언트 소켓 종료
    close(client_socket);
    return NULL;
}

int start_server() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t thread_id;

    // 서버 소켓 생성
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    // 소켓 바인딩
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return -1;
    }

    // 소켓 리스닝
    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        return -1;
    }

    while (1) {
        // 클라이언트 연결 기다리기
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) < 0) {
            perror("Client accept failed");
            continue;
        }

        // 클라이언트 요청을 새로운 스레드에서 처리
        if (pthread_create(&thread_id, NULL, handle_client, (void *)&client_socket) != 0) {
            perror("Thread creation failed");
        }
    }

    return 0;
}