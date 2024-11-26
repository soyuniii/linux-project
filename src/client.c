#include <stdio.h>
#include <stdlib.h>

void show_search_menu() {
    printf("검색 메뉴입니다. 키워드를 입력하세요:\n");
    // 검색 기능을 구현할 수 있는 부분
}

void show_recommended_movie() {
    printf("오늘의 추천 영화 😊\n");
    printf("오늘 '영화이름', 어떠세요?\n");
    printf("추천 영화 링크: \n");
    // 추천 영화에 대한 추가 기능을 구현할 수 있습니다.
}

void show_movie_list() {
    printf("\n나중에 볼 영화목록\n");
    // 나중에 볼 영화 목록 기능을 구현할 수 있습니다.
}

void show_review_board() {
    printf("\n영화 감상 게시판\n");
    // 영화 감상 게시판 관련 기능을 구현할 수 있습니다.
}

int main() {
    int choice;

    while(1) {
        printf("\n=================================\n");
        printf("  ◆ MOVIE PLACE에 오신 분들, 환영합니다. \n");
        printf("=================================\n");
        printf("1. 검색\n");
        printf("2. 오늘의 추천 영화\n");
        printf("3. 나중에 볼 영화목록\n");
        printf("4. 영화 감상 게시판\n");
        printf("5. 나가기\n");
        printf("---------------------------------\n");
        printf("메뉴를 선택하세요 >>> ");
        
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                show_search_menu();
                break;
            case 2:
                show_recommended_movie();
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