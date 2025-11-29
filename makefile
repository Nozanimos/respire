# Compilateur et flags
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -g -I$(SRC_DIR)
SDL_FLAGS = `sdl2-config --cflags --libs`
CAIRO_FLAGS = `pkg-config --cflags --libs cairo freetype2`
LIBS = -lSDL2_image -lSDL2_gfx -lSDL2_ttf -lSDL2_mixer -lm -lcjson

# Dossiers
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Trouve tous les fichiers .c dans src/
SRCS = $(wildcard $(SRC_DIR)/*.c) \
       $(wildcard $(SRC_DIR)/core/*.c) \
       $(wildcard $(SRC_DIR)/core/memory/*.c) \
       $(wildcard $(SRC_DIR)/core/error/*.c) \
       $(wildcard $(SRC_DIR)/instances/*.c) \
       $(wildcard $(SRC_DIR)/instances/whm/*.c) \
       $(wildcard $(SRC_DIR)/json_editor/*.c)
# Transforme src/fichier.c → obj/fichier.o
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
# Nom de l'exécutable final
TARGET = $(BIN_DIR)/respire

# Règle principale
all: $(TARGET)

# Lien : .o → exécutable
$(TARGET): $(OBJS)
	@echo "🔗 Édition des liens..."
	@mkdir -p $(BIN_DIR)  # Crée le dossier bin si inexistant
	$(CC) $(OBJS) -o $(TARGET) $(LIBS) $(SDL_FLAGS) $(CAIRO_FLAGS)
	@echo "✅ Compilation terminée : $(TARGET)"

# Compilation : .c → .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "📦 Compilation de $<..."
	@mkdir -p $(dir $@)  # Crée le dossier si inexistant
	$(CC) $(CFLAGS) `sdl2-config --cflags` `pkg-config --cflags cairo freetype2` -c $< -o $@

# Règles pour bear/compile_commands.json
compile_commands.json:
	bear -- make clean
	bear -- make

compiledb: compile_commands.json

# Nettoyage
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "🧹 Fichiers nettoyés"

# Recompile tout
re: clean all

# Aide
help:
	@echo "Usage:"
	@echo "  make all    - Compile le projet"
	@echo "  make clean  - Nettoie les fichiers compilés"
	@echo "  make re     - Recompile tout"
	@echo "  make help   - Affiche cette aide"

.PHONY: all clean re help compile_commands compiledb
