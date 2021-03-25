all: main

UNAME := $(shell uname)

main:
	rm -f main
ifeq ($(UNAME), Linux)
	gcc -g -Wall -Wextra -pedantic -std=c11 -fsanitize=address,undefined main.c -lSDL2 -lm -o main
endif
ifeq ($(UNAME), Darwin)
	clang -g -Wall -Wextra main.c -fsanitize=address,undefined -F/Library/Frameworks -framework SDL2 -o main
endif

release:
ifeq ($(UNAME), Linux)
	gcc -O3 -g -Wall -Wextra -pedantic -std=c11 main.c -lSDL2 -lm -o main
endif
ifeq ($(UNAME), Darwin)
	clang -O3 -Wall -Wextra main.c -F/Library/Frameworks -framework SDL2 -o main
endif

