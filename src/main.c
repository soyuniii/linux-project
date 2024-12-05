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
void show_search_menu(Movie *movies, size_t num_movies) {
    char query[256];
    printf("\n검색할 영화 제목을 입력하세요: ");
    fgets(query, sizeof(query), stdin);
    query[strcspn(query, "\n")] = 0; // 개행 문자 제거

    int found = 0;

    // 검색 결과 출력
    for (size_t i = 0; i < num_movies; i++) {
        if (strstr(movies[i].title, query) != NULL) {
            printf("%zu. %s\n", i + 1, movies[i].title);
            found = 1;
        }
    }

    if (!found) {
        printf("검색된 영화가 없습니다.\n");
    } else {
        printf("\n상세 정보를 볼 영화의 번호를 입력하세요: ");
        size_t choice;
        scanf("%zu", &choice);
        getchar();

        if (choice > 0 && choice <= num_movies) {
            Movie selected_movie = movies[choice - 1];
            printf("\n영화 제목: %s\n", selected_movie.title);
            get_trailer_link(&selected_movie);
            if (selected_movie.trailer_link) {
                printf("예고편 링크: %s\n", selected_movie.trailer_link);
            } else {
                printf("예고편을 찾을 수 없습니다.\n");
            }
        } else {
            printf("잘못된 번호입니다.\n");
        }
    }
}

/********************************** 2. 오늘의 영화 ***************************************/
void show_recommend_movie(Movie *movies, size_t num_movies) {
    if (num_movies == 0) {
        printf("영화를 가져올 수 없습니다.\n");
        return;
    }

    // 랜덤 인덱스 생성
    srand(time(NULL));
    int random_index = rand() % num_movies;

    // 추천 영화 출력
    printf("\n\n--------------------------------------\n");
    printf("오늘의 추천 영화 😊\n");
    printf("--------------------------------------\n");
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

int main() {
    int choice;

    // 영화 목록과 개수 선언
    Movie *movies = NULL;  // 영화 목록을 저장할 배열
    size_t num_movies = 0; // 영화 개수

    // TMDB API에서 영화 목록을 가져옵니다.
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
        printf("-----------------------------------------\n");
        printf("메뉴를 선택하세요 >>> ");

        scanf("%d", &choice);

        switch (choice) {
            case 1:
                show_search_menu(movies, num_movies);
                break;
            case 2:
                show_recommend_movie(movies, num_movies);
                break;
            case 3:
                //show_movie_list();
                break;
            case 4:
                //show_review_board();
                break;
            case 5:
                printf("프로그램을 종료합니다.\n");
                return 0; // 프로그램 종료
            default:
                printf("잘못된 입력입니다. 다시 시도해주세요.\n");
        }

        printf("\n계속하시겠습니까? (Y/N): ");
        char cont;
        scanf(" %c", &cont);  // 여백을 추가하여 버퍼 문제 방지
        if (cont == 'N' || cont == 'n') {
            break;
        }
    }

    printf("프로그램을 종료합니다.\n");
    return 0;
}