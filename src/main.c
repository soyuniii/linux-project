#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include "client.h"

int main() {
    // 서버 초기화 및 클라이언트 요청 처리
    if (start_server() != 0) {
        fprintf(stderr, "Server failed to start.\n");
        return 1;
    }
    return 0;
}