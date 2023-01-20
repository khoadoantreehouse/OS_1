# build an executable named movies from main.c
all: main.c 
	gcc --std=c99 -o movies main.c

clean:
	rm -f movies

test:
	gcc --std=gnu99 -o movies main.c
	./movies movies_sample_1.csv

leak-test:
	gcc --std=gnu99 -o movies main.c
	valgrind --leak-check=yes ./movies movies_sample_1.csv