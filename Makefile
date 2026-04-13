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

CLIENT_SRC = client/src/client.c
SERVER_SRC = server/src/server.c
NETWORK_SRC = lib/src/network.c
GAME_MAP_SRC = lib/src/game_map.c
PLAYER_MOVEMENT_SRC = lib/src/player_movement.c
LOBBY_SRC = lib/src/lobby.c
GAME_SRC = lib/src/game.c

CLIENT_OBJ = $(OBJDIR)/client.o
SERVER_OBJ = $(OBJDIR)/server.o
NETWORK_OBJ = $(OBJDIR)/network.o
GAME_MAP_OBJ = $(OBJDIR)/game_map.o
PLAYER_MOVEMENT_OBJ = $(OBJDIR)/player_movement.o
LOBBY_OBJ = $(OBJDIR)/lobby.o
GAME_OBJ = $(OBJDIR)/game.o

PLAYER_MOVEMENT_TEST_OBJ = $(OBJDIR)/test_player_movement.o

CFLAGS  = -g -c -Ilib/include
LDFLAGS = -lSDL2main -lSDL2 -lSDL2_image -lSDL2_net -lm -lSDL2_ttf

SERVER_OUT = build/server
CLIENT_OUT = build/client
PLAYER_MOVEMENT_TEST_OUT = build/test_player_movement

# ─── Per plattform ───────────────────────────────────────
ifeq ($(PLATFORM),mac)
    CC      = clang
    CFLAGS += -I/opt/homebrew/include
    LDFLAGS += -L/opt/homebrew/lib

else ifeq ($(PLATFORM),linux)
    CC      = gcc
    CFLAGS += -I/usr/include

else ifeq ($(PLATFORM),windows)
    CC       = gcc
    CFLAGS  += -I/mingw64/include
    LDFLAGS += -L/mingw64/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_net -lm -lSDL2_ttf
endif

# ─── Bygg-regler ─────────────────────────────────────────
$(OBJDIR):
	mkdir -p $(OBJDIR)

all: $(SERVER_OUT) $(CLIENT_OUT)

$(CLIENT_OBJ): $(CLIENT_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_OBJ)

$(SERVER_OBJ): $(SERVER_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_OBJ)

$(NETWORK_OBJ): $(NETWORK_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) $(NETWORK_SRC) -o $(NETWORK_OBJ)

$(GAME_MAP_OBJ): $(GAME_MAP_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) $(GAME_MAP_SRC) -o $(GAME_MAP_OBJ)

$(PLAYER_MOVEMENT_OBJ): $(PLAYER_MOVEMENT_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) $(PLAYER_MOVEMENT_SRC) -o $(PLAYER_MOVEMENT_OBJ)

$(LOBBY_OBJ): $(LOBBY_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) $(LOBBY_SRC) -o $(LOBBY_OBJ)

$(GAME_OBJ): $(GAME_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) $(GAME_SRC) -o $(GAME_OBJ)

$(PLAYER_MOVEMENT_TEST_OBJ): test_files/test_player_movement.c | $(OBJDIR)
	$(CC) $(CFLAGS) test_files/test_player_movement.c -o $(PLAYER_MOVEMENT_TEST_OBJ)

$(CLIENT_OUT): $(CLIENT_OBJ) $(NETWORK_OBJ) $(GAME_MAP_OBJ) $(PLAYER_MOVEMENT_OBJ) $(LOBBY_OBJ) $(GAME_OBJ)
	$(CC) $(CLIENT_OBJ) $(NETWORK_OBJ) $(GAME_MAP_OBJ) $(PLAYER_MOVEMENT_OBJ) $(LOBBY_OBJ) $(GAME_OBJ) -o $(CLIENT_OUT) $(LDFLAGS)

$(SERVER_OUT): $(SERVER_OBJ) $(NETWORK_OBJ)
	$(CC) $(SERVER_OBJ) $(NETWORK_OBJ) -o $(SERVER_OUT) $(LDFLAGS)

$(PLAYER_MOVEMENT_TEST_OUT): $(PLAYER_MOVEMENT_TEST_OBJ) $(PLAYER_MOVEMENT_OBJ) $(GAME_MAP_OBJ)
	$(CC) $(PLAYER_MOVEMENT_TEST_OBJ) $(PLAYER_MOVEMENT_OBJ) $(GAME_MAP_OBJ) -o $(PLAYER_MOVEMENT_TEST_OUT) $(LDFLAGS)

# ─── Kör-targets ─────────────────────────────────────────
run: $(CLIENT_OUT)
	./$(CLIENT_OUT)

# ─── Städa ───────────────────────────────────────────────
clean:
ifeq ($(PLATFORM),windows)
	del /Q $(CLIENT_OUT) $(SERVER_OUT)
	rmdir /S /Q $(OBJDIR)
else
	rm -rf $(OBJDIR)
endif