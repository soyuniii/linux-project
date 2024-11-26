CC = gcc
CFLAGS = -Wall -g
LIBS = -lcurl -lssl -lcrypto -lpthread

all: linux-project

linux-project: src/main.c src/server.c src/client.c src/movie_manager.c src/profile_manager.c src/security.c
    $(CC) $(CFLAGS) $^ -o linux-project $(LIBS)

clean:
    rm -f linux-project