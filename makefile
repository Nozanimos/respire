all:
	gcc ./src/*.c \
    -Wall -Wextra -pedantic \
    -o ./bin/respire \
    -lm \
    -lSDL2_image \
    -lSDL2_gfx \
    -lSDL2_ttf \
    `sdl2-config --cflags --libs`
