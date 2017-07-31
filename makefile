all: kerrshell

kerrshell: kerrshell.c linkedList.c utility.c
	gcc -g kerrshell.c linkedList.c utility.c -o ssh

clean:
	rm -f ssh
