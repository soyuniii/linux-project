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
    int id;             // 영화 ID
} Movie;

// 응답 데이터를 저장할 구조체
struct MemoryStruct {
    char *memory;
    size_t size;
};

// 함수 선언
size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data);
int get_tmdb_data(Movie **movies, size_t *num_movies);
void get_trailer_link(Movie *movie);
void search_movie(const char *api_key, const char *query, char **result_data);
void display_movie_details(const char *json_data, const char *api_key);

// TMDB API 키 (전역 변수로 설정) -> 개인 API라서 환경변수로 실행하려면 이 코드를 지우면 됨
const char *api_key = "3cea7405c8faed2ce6b9df6d5998e853";

// 응답 데이터를 저장하는 콜백 함수
size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1); // +1 for null terminator
    if (mem->memory == NULL) {
        printf("Not enough memory!\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0; // Null terminator 추가

    return realsize;
}

// TMDB API에서 데이터를 가져오는 함수
int get_tmdb_data(Movie **movies, size_t *num_movies) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);  // 응답 데이터를 저장할 메모리 할당
    chunk.size = 0;  // 초기화

    const char *api_key = "3cea7405c8faed2ce6b9df6d5998e853";
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
                    int id = (int)json_integer_value(json_object_get(movie_json, "id"));

                    // 영화 제목과 ID를 movies 배열에 저장
                    (*movies)[i].title = strdup(title);
                    (*movies)[i].id = id;
                    if ((*movies)[i].title == NULL) {
                        fprintf(stderr, "Memory allocation failed for movie title at index %zu\n", i);
                        json_decref(root);
                        return 1;
                    }

                    (*movies)[i].trailer_link = NULL;
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

// 영화 예고편을 가져오는 함수
void get_trailer_link(Movie *movie) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);  // 응답 데이터를 저장할 메모리 할당
    chunk.size = 0;  // 초기화

    const char *api_key = getenv("TMDB_API_KEY");
    if (api_key == NULL) {
        printf("API Key not found\n");
        return;
    }

    char url[512];
    snprintf(url, sizeof(url), "https://api.themoviedb.org/3/movie/%d/videos?api_key=%s&language=ko-KR", movie->id, api_key);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // JSON 파싱
            json_error_t error;
            json_t *root = json_loads(chunk.memory, 0, &error);

            if (!root) {
                fprintf(stderr, "JSON decoding failed: %s\n", error.text);
                free(chunk.memory);
                return;
            }

            json_t *results = json_object_get(root, "results");
            if (json_is_array(results) && json_array_size(results) > 0) {
                // 첫 번째 예고편의 key를 가져와서 YouTube 링크 생성
                const char *key = json_string_value(json_object_get(json_array_get(results, 0), "key"));
                if (key != NULL) {
                    movie->trailer_link = malloc(256);
                    snprintf(movie->trailer_link, 256, "https://www.youtube.com/watch?v=%s \n", key);
                }
            }

            json_decref(root);
        }

        curl_easy_cleanup(curl);
    }

    free(chunk.memory);
    curl_global_cleanup();
}

/********************************** 1. 검색 ***************************************/

// 영화 검색 함수
void search_movie(const char *api_key, const char *query, char **result_data) {
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
            *result_data = strdup(chunk.memory); // 결과 저장
        } else {
            fprintf(stderr, "curl_easy_perform() 실패: %s\n", curl_easy_strerror(res));
        }

        curl_free(encoded_query); // URL 인코딩된 문자열 해제
        curl_easy_cleanup(curl);
    }
    free(chunk.memory); // 메모리 해제
    curl_global_cleanup();
}

// 영화 상세 정보 출력 함수
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

            
        } else {
            printf("잘못된 번호입니다.\n");
        }
    } else {
        printf("검색 결과가 없습니다.\n");
    }
    json_decref(root); // JSON 객체 해제
}

// 검색 메뉴 함수
void show_search_menu(Movie *movies, size_t num_movies) {
    char query[256];
    printf("\n검색할 영화 제목을 입력하세요: ");
    fgets(query, sizeof(query), stdin);
    query[strcspn(query, "\n")] = 0; // 개행 문자 제거

    char *json_data = NULL;
    search_movie(api_key, query, &json_data); // 전역 변수로 설정된 API 키 사용

    if (json_data) {
        display_movie_details(json_data, api_key);
        free(json_data);
    }
}

/********************************** 2. 오늘의 영화 ***************************************/

// 추천 영화 출력 함수
void show_recommend_movie(Movie *movies, size_t num_movies) {
    if (num_movies == 0) {
        printf("영화를 가져올 수 없습니다.\n");
        return;
    }

    // 랜덤 인덱스 생성
    srand(time(NULL));
    int random_index = rand() % num_movies;

    // 추천 영화 출력
    printf("\n\n———————————————————\n");
    printf("오늘의 추천 영화 😊\n");
    printf("———————————————————\n");
    printf("오늘 '%s', 어떠세요?\n", movies[random_index].title);

    // 예고편 링크 출력
    get_trailer_link(&movies[random_index]);
    if (movies[random_index].trailer_link != NULL) {
        printf("\n▼ 예고편을 감상해보세요! \n");
        printf("%s\n", movies[random_index].trailer_link);
    } else {
        printf("이 영화는 예고편이 제공되어있지 않아요 ㅜ.ㅜ.\n");
    }
}

/********************************** main함수 ***************************************/

// main 함수
int main() {
    int choice;
    Movie *movies = NULL;
    size_t num_movies = 0;

    // 영화 데이터를 가져옵니다.
    if (get_tmdb_data(&movies, &num_movies) != 0) {
        printf("영화 데이터를 가져오는 데 실패했습니다.\n");
        return 1;
    }

    while (1) {
        printf("\n========================================\n");
        printf("    ◆ MOVIE PLACE에 오신 분들, 환영합니다. \n");
        printf("========================================\n");
        printf("1. 검색\n");
        printf("2. 오늘의 추천 영화\n");
        printf("3. 나중에 볼 영화목록\n");
        printf("4. 영화 감상 게시판\n");
        printf("5. 나가기\n");
        printf("————————————————————\n");
        printf("메뉴를 선택하세요 >>> ");

        scanf("%d", &choice);
        getchar(); // 버퍼 정리

        switch (choice) {
            case 1:
                show_search_menu(movies, num_movies);
                break;
            case 2:
                show_recommend_movie(movies, num_movies);
                break;
            case 3:
                // TODO: 나중에 볼 영화목록 기능 추가
                break;
            case 4:
                // TODO: 영화 감상 게시판 기능 추가
                break;
            case 5:
                printf("프로그램을 종료합니다.\n");
                return 0;
            default:
                printf("잘못된 입력입니다. 다시 시도해주세요.\n");
        }

        printf("\n계속하시겠습니까? (Y/N): ");
        char cont;
        scanf(" %c", &cont); // 여백 추가로 버퍼 문제 방지
        if (cont == 'N' || cont == 'n') {
            break;
        }
    }

    printf("프로그램을 종료합니다.\n");
    return 0;
}