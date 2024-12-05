#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

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
        fprintf(file, "%s\n", movie_title);  // 영화 제목 저장
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

// 메인 함수
int main() {
    int choice;
    char movie_title[256];

    while (1) {
        printf("\n========================\n");
        printf("   나중에 볼 영화목록\n");
        printf("========================\n");
        printf(" 1. 저장하기\n");
        printf(" 2. 불러오기\n");
        printf(" 3. 목록 편집\n");
        printf(" 4. 나가기\n");
        printf("------------------------\n");
        printf("옵션을 선택해주세요>>> ");
        scanf("%d", &choice);
        getchar(); // 입력 버퍼 비우기

        switch (choice) {
            case 1:
                printf("저장할 영화 제목을 입력하세요>>> ");
                fgets(movie_title, sizeof(movie_title), stdin);
                movie_title[strcspn(movie_title, "\n")] = '\0'; // 줄바꿈 제거
                save_movie_to_profile(movie_title);
                break;
            case 2:
                load_movies_from_profile();
                break;
            case 3:
                edit_movies_in_profile();
                break;
            case 4:
                printf("프로그램을 종료합니다.\n");
                return 0;
            default:
                printf("잘못된 옵션입니다. 다시 시도해주세요.\n");
        }
    }
}