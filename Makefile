all: BackItUp
BackItUp: main.c
	gcc -g -o BackItUp main.c -lpthread
