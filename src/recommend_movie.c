#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <jansson.h>
#include <curl/curl.h>

// 영화 데이터를 저장할 구조체
typedef struct {
    char *title;        // 영화 제목
    char *trailer_link; // 예고편 링크
} Movie;

// 응답 데이터를 저장할 구조체
struct MemoryStruct {
    char *memory;
    size_t size;
};

// 응답 데이터를 저장하는 콜백 함수
size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;

    // 메모리 크기 재할당
    mem->memory = realloc(mem->memory, mem->size + realsize + 1);  // +1 for null terminator
    if (mem->memory == NULL) {
        printf("Not enough memory!\n");
        return 0;
    }

    // 응답 데이터를 memory 배열에 복사
    memcpy(&(mem->memory[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;  // 문자열 끝에 null terminator 추가

    return realsize;
}

// TMDB API에서 데이터를 가져오는 함수
int get_tmdb_data(Movie **movies, size_t *num_movies) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);  // 응답 데이터를 저장할 메모리 할당
    chunk.size = 0;  // 초기화

    const char *api_key = getenv("TMDB_API_KEY");
    if (api_key == NULL) {
        return 1;
    }

    char url[512];
    snprintf(url, sizeof(url), "https://api.themoviedb.org/3/movie/popular?api_key=%s&language=ko-KR", api_key);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return 1;
        } else {
            // 응답 받은 데이터 출력
            // printf("Response: %s\n", chunk.memory);

            // JSON 파싱
            json_error_t error;
            json_t *root = json_loads(chunk.memory, 0, &error);

            if (!root) {
                fprintf(stderr, "JSON decoding failed: %s\n", error.text);
                free(chunk.memory);
                return 1;
            }

            json_t *results = json_object_get(root, "results");
            if (json_is_array(results)) {
                *num_movies = json_array_size(results);
                *movies = malloc(*num_movies * sizeof(Movie));
                if (*movies == NULL) {
                    fprintf(stderr, "Memory allocation failed for movies\n");
                    json_decref(root);
                    return 1;
                }

                for (size_t i = 0; i < *num_movies; i++) {
                    json_t *movie_json = json_array_get(results, i);
                    const char *title = json_string_value(json_object_get(movie_json, "title"));

                    // 영화 제목을 movies 배열에 저장
                    (*movies)[i].title = strdup(title);
                    if ((*movies)[i].title == NULL) {
                        fprintf(stderr, "Memory allocation failed for movie title at index %zu\n", i);
                        json_decref(root);
                        return 1;
                    }

                    (*movies)[i].trailer_link = strdup("예고편 링크 (현재는 TMDb API에서 제공되지 않음)");
                    if ((*movies)[i].trailer_link == NULL) {
                        fprintf(stderr, "Memory allocation failed for trailer link at index %zu\n", i);
                        json_decref(root);
                        return 1;
                    }
                }
            }

            json_decref(root);
        }

        curl_easy_cleanup(curl);
    }

    free(chunk.memory);
    curl_global_cleanup();
    return 0;
}

// 랜덤으로 추천 영화를 출력하는 함수
void show_recommend_movie(Movie *movies, size_t num_movies) {
    if (num_movies == 0) {
        printf("영화를 가져올 수 없습니다.\n");
        return;
    }

    // 랜덤 인덱스 생성
    srand(time(NULL));
    int random_index = rand() % num_movies;

    // 추천 영화 출력
    printf("\n\n---------------------------------\n");
    printf("오늘의 추천 영화 😊\n");
    printf("오늘 '%s', 어떠세요?\n", movies[random_index].title);
}

int main() {
    Movie *movies = NULL;  // 영화 목록을 저장할 배열
    size_t num_movies = 0; // 영화 개수

    // 환경 변수에서 API 키 가져오기
    char *api_key = getenv("TMDB_API_KEY");

    if (api_key == NULL) {
        printf("API Key not found\n");
        return 1;
    }

    // TMDB API 데이터를 가져옵니다
    if (get_tmdb_data(&movies, &num_movies) != 0) {
        printf("Failed to get TMDb data\n");
        return 1;
    }

    // 추천 영화 출력
    show_recommend_movie(movies, num_movies);

    // 메모리 해제
    for (size_t i = 0; i < num_movies; i++) {
        free(movies[i].title);
        free(movies[i].trailer_link);
    }
    free(movies);

    return 0;
}