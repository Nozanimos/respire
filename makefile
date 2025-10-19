# Compilateur et flags
CC = gcc
CFLAGS = -Wall -Wextra -pedantic
SDL_FLAGS = `sdl2-config --cflags --libs`
LIBS = -lSDL2_image -lSDL2_gfx -lSDL2_ttf -lm

# Dossiers
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Trouve tous les fichiers .c dans src/
SRCS = $(wildcard $(SRC_DIR)/*.c)
# Transforme src/fichier.c → obj/fichier.o
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
# Nom de l'exécutable final
TARGET = $(BIN_DIR)/respire

# Règle principale
all: $(TARGET)

# Lien : .o → exécutable
$(TARGET): $(OBJS)
	@echo "🔗 Édition des liens..."
	$(CC) $(OBJS) -o $(TARGET) $(LIBS) $(SDL_FLAGS)
	@echo "✅ Compilation terminée : $(TARGET)"

# Compilation : .c → .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "📦 Compilation de $<..."
	@mkdir -p $(OBJ_DIR)  # Crée le dossier si inexistant
	$(CC) $(CFLAGS) -c $< -o $@ $(SDL_FLAGS)

# Nettoyage
clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET)
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

.PHONY: all clean re help
