# build an executable named movies from main.c
all: main.c 
	gcc --std=c99 -o smallsh main.c

clean:
	rm -f smallsh

test:
	gcc --std=c99 -o smallsh main.c
	./smalsh
	
leak-test:
	gcc --std=c99 -o smallsh main.c
	valgrind --leak-check=yes ./smalsh