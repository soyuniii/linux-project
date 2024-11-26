#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

// 응답 데이터를 저장할 구조체
struct MemoryStruct {
    char *memory;
    size_t size;
};

// 응답 데이터를 저장하는 콜백 함수
size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        printf("Not enough memory!\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// TMDB API에서 데이터를 가져오는 함수
void get_tmdb_data(const char *api_key) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);  // 응답 데이터를 저장할 메모리 할당
    chunk.size = 0;  // 초기화

    // TMDB API URL 설정
    char url[512];
    snprintf(url, sizeof(url), "https://api.themoviedb.org/3/movie/popular?api_key=%s", api_key);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        // API 요청 실행
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // 응답 받은 데이터 출력
            printf("Response: %s\n", chunk.memory);

            // JSON 파싱
            json_error_t error;
            json_t *root = json_loads(chunk.memory, 0, &error);

            if (!root) {
                fprintf(stderr, "JSON decoding failed: %s\n", error.text);
                free(chunk.memory);
                return;
            }

            json_t *results = json_object_get(root, "results");
            if (json_is_array(results)) {
                json_t *movie = json_array_get(results, 0);
                const char *title = json_string_value(json_object_get(movie, "title"));
                printf("First movie title: %s\n", title);
            }

            // JSON 객체 해제
            json_decref(root);
        }

        // 리소스 해제
        curl_easy_cleanup(curl);
    }

    free(chunk.memory);
    curl_global_cleanup();
}

int main() {
    // 환경 변수에서 API 키 가져오기
    char *api_key = getenv("TMDB_API_KEY");

    if (api_key == NULL) {
        printf("API Key not found\n");
        return 1;
    }

    // TMDB API 데이터를 가져옵니다
    get_tmdb_data(api_key);

    return 0;
}