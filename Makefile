
.PHONY: all
all:
	gcc -I. -std=c11 -g -Wall main.c src/websocket.c -o main
