#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <curl/curl.h>
#include <jansson.h>

// 영화 예고편 URL 가져오기
void get_movie_trailer(const char *api_key, int movie_id) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1); // 초기 메모리 할당
    chunk.size = 0;

    // TMDB API URL 설정
    char url[512];
    snprintf(url, sizeof(url), "https://api.themoviedb.org/3/movie/%d/videos?api_key=%s&language=ko-KR", movie_id, api_key);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        // CURL 요청 실행
        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            json_error_t error;
            json_t *root = json_loads(chunk.memory, 0, &error); // JSON 파싱

            if (root) {
                json_t *results = json_object_get(root, "results"); // "results" 배열 가져오기
                if (json_is_array(results) && json_array_size(results) > 0) {
                    json_t *video = json_array_get(results, 0); // 첫 번째 결과 가져오기
                    const char *key = json_string_value(json_object_get(video, "key"));
                    const char *site = json_string_value(json_object_get(video, "site"));

                    // YouTube 링크 출력
                    if (key && strcmp(site, "YouTube") == 0) {
                        printf("예고편: https://www.youtube.com/watch?v=%s\n", key);
                    } else {
                        printf("예고편을 찾을 수 없습니다.\n");
                    }
                } else {
                    printf("예고편을 찾을 수 없습니다.\n");
                }
                json_decref(root); // JSON 객체 해제
            } else {
                fprintf(stderr, "JSON decoding failed: %s\n", error.text);
            }
        } else {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
    }
    free(chunk.memory); // 메모리 해제
    curl_global_cleanup();
}

// 영화 검색 함수
void search_movie(const char *api_key, const char *query, int pipe_fd) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1); // 초기 메모리 할당
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // URL 인코딩
        char *encoded_query = curl_easy_escape(curl, query, 0);
        if (!encoded_query) {
            fprintf(stderr, "URL 인코딩 실패\n");
            curl_easy_cleanup(curl);
            return;
        }

        // TMDB API URL 설정
        char url[512];
        snprintf(url, sizeof(url), "https://api.themoviedb.org/3/search/movie?api_key=%s&query=%s&language=ko-KR", api_key, encoded_query);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        // CURL 요청 실행
        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            write(pipe_fd, chunk.memory, chunk.size); // 파이프에 결과 쓰기
        } else {
            fprintf(stderr, "curl_easy_perform() 실패: %s\n", curl_easy_strerror(res));
        }

        curl_free(encoded_query); // URL 인코딩된 문자열 해제
        curl_easy_cleanup(curl);
    }
    free(chunk.memory); // 메모리 해제
    curl_global_cleanup();
}



// 영화 상세 정보를 표시하는 함수
void display_movie_details(const char *json_data, const char *api_key) {
    json_error_t error;
    json_t *root = json_loads(json_data, 0, &error); // JSON 파싱

    if (!root) {
        fprintf(stderr, "JSON decoding failed: %s\n", error.text);
        return;
    }

    json_t *results = json_object_get(root, "results"); // "results" 배열 가져오기
    if (json_is_array(results)) {
        size_t index;
        json_t *movie;

        printf("\n--- 검색 결과 ---\n");
        json_array_foreach(results, index, movie) { // 영화 리스트 출력
            const char *title = json_string_value(json_object_get(movie, "title"));
            if (title) {
                printf("%zu. %s\n", index + 1, title);
            }
        }

        printf("\n상세 정보를 볼 영화의 번호를 입력하세요: ");
        size_t choice;
        scanf("%zu", &choice);
        getchar();

        if (choice > 0 && choice <= json_array_size(results)) {
            movie = json_array_get(results, choice - 1);
            const char *title = json_string_value(json_object_get(movie, "title"));
            const char *overview = json_string_value(json_object_get(movie, "overview"));
            const char *release_date = json_string_value(json_object_get(movie, "release_date"));
            const char *poster_path = json_string_value(json_object_get(movie, "poster_path"));
            int movie_id = json_integer_value(json_object_get(movie, "id"));

            // 영화 상세 정보 출력
            printf("\n--- 상세 정보 ---\n");
            printf("\n");
            printf("제목: %s\n", title ? title : "정보 없음");
            printf("\n");
            printf("줄거리: %s\n", overview ? overview : "정보 없음");
            printf("\n");
            printf("개봉일: %s\n", release_date ? release_date : "정보 없음");
            printf("\n");
            if (poster_path) {
                printf("포스터: https://image.tmdb.org/t/p/w500%s\n", poster_path);
            } else {
                printf("포스터를 찾을 수 없습니다.\n");
            }
            printf("\n");

            // 예고편 정보 가져오기
            get_movie_trailer(api_key, movie_id);
        } else {
            printf("잘못된 번호입니다.\n");
        }
    } else {
        printf("검색 결과가 없습니다.\n");
    }
    json_decref(root); // JSON 객체 해제
}

// 메인 함수
int main() {
    char *api_key = "3cea7405c8faed2ce6b9df6d5998e853"; // 환경 변수에서 API 키 가져오기
    if (api_key == NULL) {
        printf("API Key not found\n");
        return 1;
    }

    char query[256];
    printf("검색할 영화 제목을 입력하세요: ");
    fgets(query, sizeof(query), stdin);
    query[strcspn(query, "\n")] = 0; // 개행 문자 제거

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) { // 파이프 생성
        perror("pipe failed");
        return 1;
    }

    pid_t pid = fork(); // 프로세스 분기

    if (pid == -1) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) { // 자식 프로세스
        close(pipe_fd[1]); // 쓰기 닫기
        char buffer[65536];
        read(pipe_fd[0], buffer, sizeof(buffer)); // 파이프에서 데이터 읽기
        close(pipe_fd[0]); // 읽기 닫기

        display_movie_details(buffer, api_key); // 영화 상세 정보 표시
    } else { // 부모 프로세스
        close(pipe_fd[0]); // 읽기 닫기
        search_movie(api_key, query, pipe_fd[1]); // 영화 검색 실행
        close(pipe_fd[1]); // 쓰기 닫기

        wait(NULL); // 자식 프로세스 대기
    }

    return 0;
}