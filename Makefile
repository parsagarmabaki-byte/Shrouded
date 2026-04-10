# =========================
# Project: shrouded
# SDL2 + SDL2_image
# =========================

# ─── Detektera OS ────────────────────────────────────────
ifeq ($(OS),Windows_NT)
    PLATFORM = windows
else
    UNAME := $(shell uname -s)
    ifeq ($(UNAME),Darwin)
        PLATFORM = mac
    else
        PLATFORM = linux
    endif
endif

# ─── Gemensamt ───────────────────────────────────────────
OBJDIR = build

CLIENT_SRC           = client/src/client.c
SERVER_SRC           = server/src/server.c
PLAYER_MOVEMENT_SRC  = lib/src/player_movement.c
GAME_MAP_SRC         = lib/src/game_map.c

CLIENT_OBJ           = $(OBJDIR)/client.o
SERVER_OBJ           = $(OBJDIR)/server.o
PLAYER_MOVEMENT_OBJ  = $(OBJDIR)/player_movement.o
GAME_MAP_OBJ         = $(OBJDIR)/game_map.o

SERVER_OUT           = build/server
CLIENT_OUT           = build/client
PLAYER_MOVEMENT_OUT  = build/player_movement
GAME_MAP_OUT         = build/game_map

CFLAGS  = -g -Ilib/include
COMPILE = $(CFLAGS) -c
LDFLAGS = -lSDL2main -lSDL2 -lSDL2_image -lSDL2_net -lSDL2_ttf -lm

# ─── Per plattform ───────────────────────────────────────
ifeq ($(PLATFORM),mac)
    CC       = gcc
    CFLAGS  += -I/opt/homebrew/include
    LDFLAGS += -L/opt/homebrew/lib

else ifeq ($(PLATFORM),linux)
    CC       = gcc
    CFLAGS  += -I/usr/include

else ifeq ($(PLATFORM),windows)
    CC       = gcc
    CFLAGS  += -I/mingw64/include
    LDFLAGS += -L/mingw64/lib -lmingw32
endif

# ─── Bygg-regler ─────────────────────────────────────────
$(OBJDIR):
	mkdir -p $(OBJDIR)

all: $(SERVER_OUT) $(CLIENT_OUT)

# ─── Objektfiler ─────────────────────────────────────────
$(CLIENT_OBJ): $(CLIENT_SRC) | $(OBJDIR)
	$(CC) $(COMPILE) $(CLIENT_SRC) -o $(CLIENT_OBJ)

$(SERVER_OBJ): $(SERVER_SRC) | $(OBJDIR)
	$(CC) $(COMPILE) $(SERVER_SRC) -o $(SERVER_OBJ)

$(PLAYER_MOVEMENT_OBJ): $(PLAYER_MOVEMENT_SRC) | $(OBJDIR)
	$(CC) $(COMPILE) $(PLAYER_MOVEMENT_SRC) -o $(PLAYER_MOVEMENT_OBJ)

$(GAME_MAP_OBJ): $(GAME_MAP_SRC) | $(OBJDIR)
	$(CC) $(COMPILE) $(GAME_MAP_SRC) -o $(GAME_MAP_OBJ)

# ─── Körbara filer ───────────────────────────────────────
$(CLIENT_OUT): $(CLIENT_OBJ)
	$(CC) $(CLIENT_OBJ) -o $(CLIENT_OUT) $(LDFLAGS)

$(SERVER_OUT): $(SERVER_OBJ)
	$(CC) $(SERVER_OBJ) -o $(SERVER_OUT) $(LDFLAGS)

$(PLAYER_MOVEMENT_OUT): $(PLAYER_MOVEMENT_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) $(PLAYER_MOVEMENT_SRC) -o $(PLAYER_MOVEMENT_OUT) $(LDFLAGS)

$(GAME_MAP_OUT): $(GAME_MAP_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) $(GAME_MAP_SRC) -o $(GAME_MAP_OUT) $(LDFLAGS)

# ─── Snabbkommandon ──────────────────────────────────────
build_player_movement: $(PLAYER_MOVEMENT_OUT)

build_game_map: $(GAME_MAP_OUT)

player_movement: $(PLAYER_MOVEMENT_OUT)
	./$(PLAYER_MOVEMENT_OUT)

game_map: $(GAME_MAP_OUT)
	./$(GAME_MAP_OUT)

run: $(CLIENT_OUT)
	./$(CLIENT_OUT)

# ─── Clean ───────────────────────────────────────────────
clean:
ifeq ($(PLATFORM),windows)
	del /Q $(CLIENT_OUT) $(SERVER_OUT) $(PLAYER_MOVEMENT_OUT) $(GAME_MAP_OUT)
	rmdir /S /Q $(OBJDIR)
else
	rm -rf $(OBJDIR)
endif