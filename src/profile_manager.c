//사용자가 찜한 영화를 저장하고 불러오는 기능을 담당
#include <stdio.h>
#include <string.h>

void save_movie_to_profile(const char *movie_title) {
    FILE *file = fopen("data/profile.txt", "a");
    if (file != NULL) {
        fprintf(file, "%s\n", movie_title);  // 영화 제목 저장
        fclose(file);
    } else {
        printf("Error opening profile file.\n");
    }
}

void load_movies_from_profile() {
    FILE *file = fopen("data/profile.txt", "r");
    if (file != NULL) {
        char movie_title[256];
        printf("Your saved movies:\n");
        while (fgets(movie_title, sizeof(movie_title), file)) {
            printf("%s", movie_title);
        }
        fclose(file);
    } else {
        printf("Error opening profile file.\n");
    }
}