
.PHONY: all
all:
	gcc -std=c11 -g -Wall main.c websocket.c -o main
