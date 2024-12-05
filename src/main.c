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
void show_search_menu() {
    printf("\n\n------------------------------------\n");
    printf("영화 검색:\n");
    printf("-------------------------------------\n");
    // 검색 기능 구현
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

/********************************** 3.나중에 볼 영화 목록 ***************************************/

// 중복 저장 방지
int is_movie_in_profile(const char *movie_title) {
    FILE *file = fopen("data/profile.txt", "r");
    if (file != NULL) {
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\n")] = '\0'; // 줄바꿈 제거
            if (strcmp(line, movie_title) == 0) {
                fclose(file);
                return 1; // 중복 발견
            }
        }
        fclose(file);
    }
    return 0; // 중복 아님
}

// 영화 저장
void save_movie_to_profile(const char *movie_title) {
    if (is_movie_in_profile(movie_title)) {
        printf("영화 '%s'은/는 이미 저장되었어요!\n", movie_title);
        return;
    }

    FILE *file = fopen("data/profile.txt", "a");
    if (file != NULL) {
        fprintf(file, "%s\n", movie_title);  // 제목 저장
        fclose(file);
        printf("영화 '%s'이/가 저장되었습니다!\n", movie_title);
    } else {
        printf("파일 열기에 실패했습니다.\n");
    }
}

// 영화 목록 불러오기
void load_movies_from_profile() {
    FILE *file = fopen("data/profile.txt", "r");
    if (file != NULL) {
        char movie_title[256];
        int movie_count = 0;

        printf("나의 찜하기 목록:\n");
        while (fgets(movie_title, sizeof(movie_title), file)) {
            movie_count++;
            printf("%s", movie_title);
        }
        fclose(file);

        if (movie_count == 0) {
            printf("아직 찜한 영화가 없어요! 나중에 볼 영화를 찜해주세요 :)\n"); // 찜한 영화가 없을 경우 안내 메시지
        }
    } else {
        printf("파일 열기에 실패했습니다.\n");
    }
}

// 특정 영화 삭제
void delete_movie_from_profile(const char *movie_title) {
    FILE *file = fopen("data/profile.txt", "r");
    FILE *temp_file = fopen("data/temp.txt", "w");

    if (file != NULL && temp_file != NULL) {
        char line[256];
        int found = 0;

        while (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\n")] = '\0'; // 줄바꿈 제거
            if (strcmp(line, movie_title) != 0) {
                fprintf(temp_file, "%s\n", line); // 영화 제목 복사
            } else {
                found = 1;
            }
        }
        fclose(file);
        fclose(temp_file);

        if (found) {
            remove("data/profile.txt"); // 원본 파일 삭제
            rename("data/temp.txt", "data/profile.txt"); // 임시 파일을 원본으로 교체
            printf("영화 '%s'이/가 삭제되었습니다!\n", movie_title);
        } else {
            printf("영화 '%s'을/를 찾을 수 없습니다!\n", movie_title);
            remove("data/temp.txt"); // 삭제 대상이 없으므로 임시 파일 제거
        }
    } else {
        printf("파일 열기에 실패했습니다.\n");
    }
}

// 특정 영화 수정
void modify_movie_in_profile(const char *old_title, const char *new_title) {
    FILE *file = fopen("data/profile.txt", "r");
    FILE *temp_file = fopen("data/temp.txt", "w");

    if (file != NULL && temp_file != NULL) {
        char line[256];
        int found = 0;

        while (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\n")] = '\0'; // 줄바꿈 제거
            if (strcmp(line, old_title) == 0) {
                fprintf(temp_file, "%s\n", new_title); // 기존 제목을 새 제목으로 교체
                found = 1;
            } else {
                fprintf(temp_file, "%s\n", line); // 다른 영화는 그대로 복사
            }
        }
        fclose(file);
        fclose(temp_file);

        if (found) {
            remove("data/profile.txt"); // 원본 파일 삭제
            rename("data/temp.txt", "data/profile.txt"); // 임시 파일을 원본으로 교체
            printf("영화 '%s'이/가 '%s'(으)로 수정되었습니다!\n", old_title, new_title);
        } else {
            printf("영화 '%s'을/를 찾을 수 없습니다!\n", old_title);
            remove("data/temp.txt"); // 수정 대상이 없으므로 임시 파일 제거
        }
    } else {
        printf("파일 열기에 실패했습니다.\n");
    }
}

// 편집 메뉴
void edit_movies_in_profile() {
    int edit_choice;
    char old_title[256], new_title[256];

    printf("\n========================\n");
    printf("     목록 편집 메뉴\n");
    printf("========================\n");
    printf(" 1. 삭제하기\n");
    printf(" 2. 수정하기\n");
    printf(" 3. 뒤로가기\n");
    printf("옵션을 선택해주세요>>> ");
    scanf("%d", &edit_choice);
    getchar(); // 입력 버퍼 비우기

    switch (edit_choice) {
        case 1:
            printf("삭제할 영화 제목을 입력하세요>>> ");
            fgets(old_title, sizeof(old_title), stdin);
            old_title[strcspn(old_title, "\n")] = '\0'; // 줄바꿈 제거
            delete_movie_from_profile(old_title);
            break;
        case 2:
            printf("수정할 영화 제목을 입력하세요>>> ");
            fgets(old_title, sizeof(old_title), stdin);
            old_title[strcspn(old_title, "\n")] = '\0'; // 줄바꿈 제거

            printf("새 영화 제목을 입력하세요>>> ");
            fgets(new_title, sizeof(new_title), stdin);
            new_title[strcspn(new_title, "\n")] = '\0'; // 줄바꿈 제거

            modify_movie_in_profile(old_title, new_title);
            break;
        case 3:
                return; // 메인 메뉴로 돌아가기
        default:
            printf("잘못된 옵션입니다. 다시 시도해주세요.\n");
    }
}

// 나중에 볼 영화 목록 기능 구현
void show_movie_list() {
    int choice;

    while (1) {
        printf("\n========================\n");
        printf("   나중에 볼 영화목록\n");
        printf("========================\n");
        printf(" 1. 불러오기\n");
        printf(" 2. 목록 편집\n");
        printf(" 3. 나가기\n");
        printf("------------------------\n");
        printf("옵션을 선택해주세요>>> ");
        scanf("%d", &choice);
        getchar(); // 입력 버퍼 비우기

        switch (choice) {
            case 1:
                load_movies_from_profile();
                break;
            case 2:
                edit_movies_in_profile();
                break;
            case 3:
                printf("프로그램을 종료합니다.\n");
                return;
            default:
                printf("잘못된 옵션입니다. 다시 시도해주세요.\n");
        }
    }
}

void show_review_board() {
    printf("\n영화 감상 게시판\n");
    // 영화 감상 게시판 관련 기능
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


    while(1) {
       
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
                show_search_menu();
                break;
            case 2:
                show_recommend_movie(movies, num_movies);
                break;
            case 3:
                show_movie_list();
                break;
            case 4:
                show_review_board();
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