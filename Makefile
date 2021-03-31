all: main

UNAME := $(shell uname)

main:
ifeq ($(UNAME), Linux)
	rm -f main
	gcc -g -Wall -Wextra -pedantic -std=c11 -fsanitize=address,undefined main.c -lSDL2 -lm -o main
endif
ifeq ($(UNAME), Darwin)
	rm -f main
	clang -g -Wall -Wextra main.c -fsanitize=address,undefined -F/Library/Frameworks -framework SDL2 -o main
endif
ifeq ($(OS), Windows_NT)
	gcc -g -Wall -Wextra -pedantic -std=c11 main.c -lUser32 -lGdi32 -lmingw32 -lSDL2main -lSDL2 -lm -o main
endif

release:
ifeq ($(UNAME), Linux)
	gcc -O3 -g -Wall -Wextra -pedantic -std=c11 main.c -lSDL2 -lm -o main
endif
ifeq ($(UNAME), Darwin)
	clang -O3 -Wall -Wextra main.c -F/Library/Frameworks -framework SDL2 -o main
endif
ifeq ($(OS), Windows_NT)
	gcc -O3 -Wall -Wextra -pedantic -std=c11 main.c -lmingw32 -lSDL2main -lSDL2 -lm -o main
endif

