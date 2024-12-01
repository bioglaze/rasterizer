all: main

UNAME := $(shell uname)
ARCH := $(shell uname -m)

ifeq ($(ARCH), aarch64)
ARC := -DARCH_ARM64
else
ifeq ($(ARCH), arm64)
ARC := -DARCH_ARM64
else
ARC := -DARCH_X64
endif
endif

main:
ifeq ($(UNAME), Linux)
	rm -f main
	gcc -g -Wall -Wextra -pedantic $(ARC) -std=c11 -fsanitize=address,undefined main.c -lSDL2 -lm -o main
endif
ifeq ($(UNAME), Darwin)
	rm -f main
	clang -g -Wall -Wextra main.c $(ARC) -fsanitize=address,undefined -F/Library/Frameworks -framework SDL2 -o main
endif
ifeq ($(OS), Windows_NT)
	gcc -g -Wall -Wextra -pedantic $(ARC) -std=c11 main.c -lUser32 -lGdi32 -lmingw32 -lSDL2main -lSDL2 -lm -o main
endif

release:
ifeq ($(UNAME), Linux)
	gcc -O3 -g -Wall -Wextra -pedantic $(ARC) -std=c11 -march=x86-64-v2 main.c -lSDL2 -lm -o main
endif
ifeq ($(UNAME), Darwin)
	clang -O3 -Wall -Wextra $(ARC) main.c -F/Library/Frameworks -framework SDL2 -o main
endif
ifeq ($(OS), Windows_NT)
	gcc -O3 -Wall -Wextra -pedantic $(ARC) -std=c11 main.c -lmingw32 -lSDL2main -lSDL2 -lm -o main
endif

