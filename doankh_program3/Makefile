# build an executable named movies from main.c
all: main.c 
	gcc --std=c99 -o smalsh main.c

clean:
	rm -f movies

test:
	gcc --std=c99 -o smalsh main.c
	./smalsh
	
leak-test:
	gcc --std=c99 -o smalsh main.c
	valgrind --leak-check=yes ./smalsh