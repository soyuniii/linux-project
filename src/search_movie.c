// 영화 검색 및 상세 정보 출력

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

// 영화 예고편 URL 가져오기
void get_movie_trailer(const char *api_key, int movie_id) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);
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

        // API 요청 실행
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            json_error_t error;
            json_t *root = json_loads(chunk.memory, 0, &error);

            if (!root) {
                fprintf(stderr, "JSON decoding failed: %s\n", error.text);
                free(chunk.memory);
                curl_easy_cleanup(curl);
                return;
            }

            json_t *results = json_object_get(root, "results");
            if (json_is_array(results) && json_array_size(results) > 0) {
                json_t *video = json_array_get(results, 0);
                const char *key = json_string_value(json_object_get(video, "key"));
                const char *site = json_string_value(json_object_get(video, "site"));

                if (key && strcmp(site, "YouTube") == 0) {
                    printf("예고편: https://www.youtube.com/watch?v=%s\n", key);
                } else {
                    printf("예고편을 찾을 수 없습니다.\n");
                }
            } else {
                printf("예고편을 찾을 수 없습니다.\n");
            }

            json_decref(root);
        }

        curl_easy_cleanup(curl);
    }

    free(chunk.memory);
    curl_global_cleanup();
}

// 영화 검색 함수
void search_movie(const char *api_key, const char *query) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);
    chunk.size = 0;

    // TMDB API URL 설정
    char url[512];
    snprintf(url, sizeof(url), "https://api.themoviedb.org/3/search/movie?api_key=%s&query=%s&language=ko-KR", api_key, query);

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
            json_error_t error;
            json_t *root = json_loads(chunk.memory, 0, &error);

            if (!root) {
                fprintf(stderr, "JSON decoding failed: %s\n", error.text);
                free(chunk.memory);
                curl_easy_cleanup(curl);
                return;
            }

            json_t *results = json_object_get(root, "results");
            if (json_is_array(results)) {
                printf("\n--- 검색 결과 ---\n");
                size_t index;
                json_t *movie;

                // 영화 목록 출력
                json_array_foreach(results, index, movie) {
                    const char *title = json_string_value(json_object_get(movie, "title"));

                    if (title) {
                        printf("%zu. %s\n", index + 1, title);
                    }
                }

                // 번호 선택
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

                    printf("\n--- 상세 정보 ---\n");
                    printf("영화 제목: %s\n", title ? title : "정보 없음");
                    printf("\n");
                    printf("줄거리: %s\n", overview ? overview : "정보 없음");
                    printf("\n");
                    printf("개봉일: %s\n", release_date ? release_date : "정보 없음");
                    printf("\n");

                    // 포스터 URL 생성
                    if (poster_path) {
                        printf("포스터: https://image.tmdb.org/t/p/w500%s\n", poster_path);
                        printf("\n");
                    } else {
                        printf("포스터를 찾을 수 없습니다.\n");
                    }

                    // 예고편 URL 가져오기
                    get_movie_trailer(api_key, movie_id);
                } else {
                    printf("잘못된 번호입니다.\n");
                }
            } else {
                printf("검색 결과가 없습니다.\n");
            }

            json_decref(root);
        }

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

    char query[256];
    printf("검색할 영화 제목을 입력하세요: ");
    fgets(query, sizeof(query), stdin);
    query[strcspn(query, "\n")] = 0; // 개행 문자 제거

    // 영화 검색 실행
    search_movie(api_key, query);

    return 0;
}