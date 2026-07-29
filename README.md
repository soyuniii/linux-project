
---

# 프로젝트 소개

본 프로젝트는 **클라이언트와 서버가 TCP 소켓을 통해 통신**하며 영화 정보를 제공하는 시스템입니다.

서버는 TMDB API에서 영화 데이터를 가져와 처리한 후 클라이언트에게 전달하고,
클라이언트는 검색, 영화 상세 조회, 찜 목록 관리 등의 기능을 제공합니다.

---

# 주요 기능

## 🏠 메인 화면

- 영화 검색
- 추천 영화 조회
- 나중에 볼 영화 목록 조회
- 메뉴 선택

---

## 🔍 영화 검색

TMDB API를 이용하여 다양한 조건으로 영화를 검색할 수 있습니다.

### 지원 기능

- 제목 검색
- 장르 검색
- 평점 검색
- 정렬

### 사용 기술

- TMDB API
- libcurl
- jansson(JSON Parsing)

---

## 🎬 영화 상세 정보

선택한 영화의 상세 정보를 제공합니다.

### 제공 정보

- 제목
- 장르
- 개봉일
- 평점
- 리뷰
- 예고편(Youtube) 링크

---

## ❤️ 찜하기 (나중에 볼 영화)

사용자가 선택한 영화를 로컬 파일에 저장하여 관리합니다.

### 기능

- 영화 추가
- 영화 삭제
- 영화 수정
- 목록 조회

### 구현 방식

- txt 파일 저장
- File I/O

---

## 👤 로그인 / 회원가입 (추가 기능)

사용자의 계정을 생성하고 인증하는 기능입니다.

### 회원가입

- 아이디 중복 검사
- 비밀번호 해싱(crypt())
- users.txt 저장

### 보안

- OpenSSL AES 암호화
- 파일 암호화 저장

---

---

# 🖥️ 시스템 구조

## TCP Socket 기반 Client-Server 구조

```
Client
   │
   │  영화 검색 요청
   ▼
Server
   │
   │  TMDB API 호출
   ▼
TMDB API
   │
   ▼
Server
   │
   ▼
Client
```

---

## Client

- 서버 요청
- 영화 목록 출력
- 상세 정보 출력
- 찜 목록 관리

---

## Server

- 클라이언트 요청 처리
- TMDB API 호출
- 데이터 가공 후 응답
- 멀티스레드 기반 요청 처리

---

# 🐧 Linux 활용

## 1️⃣ TCP Socket

- Client-Server 통신
- 멀티스레드 서버 구현

---

## 2️⃣ 파일 관리

### 데이터 자동 갱신

cron을 이용하여 영화 데이터를 주기적으로 업데이트합니다.

```bash
crontab -e

0 */12 * * * /path/to/movie_recommender > /dev/null 2>&1
```

### 로그 관리

Linux syslog를 이용하여 사용자 활동을 기록합니다.

```c
#include <syslog.h>

syslog(LOG_INFO, "User fetched movie recommendations");
```

---

## 3️⃣ 보안

### API Key 관리

환경변수를 이용하여 API Key를 관리합니다.

```bash
export TMDB_API_KEY="YOUR_API_KEY"
```

```c
char *api_key = getenv("TMDB_API_KEY");
```

---

### 파일 권한 설정

```bash
chmod 600 popular_movies.json
```

---

# 🛠️ 기술 스택

## Development Environment

- Ubuntu 24.04 LTS
- Visual Studio Code
- Git

---

## Language

- C

---

## Libraries

- libcurl
- jansson
- pthread
- OpenSSL

---

## External API

- TMDB API


---

# 🚀 실행 방법

### 컴파일

```bash
gcc -o movie src/*.c -lcurl -ljansson -lpthread -lssl -lcrypto
```


# 📅 개발 일정

| 주차 | 내용 |
|------|------|
| 1주차 | 프로젝트 기획 및 개발환경 구축 |
| 2주차 | TMDB API 연동 및 JSON 파싱 |
| 3주차 | UI 및 핵심 기능 구현 |
| 4주차 | 테스트 및 발표 준비 |

---

# 📌 사용 기술 요약

| 분야 | 기술 |
|------|------|
| Language | C |
| OS | Ubuntu 24.04 |
| Network | TCP Socket |
| API | TMDB API |
| Parsing | Jansson |
| HTTP | libcurl |
| Thread | pthread |
| Security | OpenSSL |
| Version Control | Git |

---

# 👥 Team

Linux Movie Recommendation Project
