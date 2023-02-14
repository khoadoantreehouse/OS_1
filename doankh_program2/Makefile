# build an executable named movies from main.c
all: main.c 
	gcc --std=c99 -o movies_by_year main.c

clean:
	rm -f movies

test:
	gcc --std=gnu99 -o movies main.c
	./movies
	
leak-test:
	gcc --std=gnu99 -o movies main.c
	valgrind --leak-check=yes ./movies