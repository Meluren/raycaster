output: src/main.o
	gcc src/main.o -lSDL2 -lm -Wall -o game

src/main.o: src/main.c
	gcc src/main.c -c -Wall -o src/main.o

clean:
	rm src/*.o
