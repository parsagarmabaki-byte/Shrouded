# =========================
# Project: shrouded
# SDL2 + SDL2_image + SDL2_net + SDL2_ttf
# =========================

.DEFAULT_GOAL := all

# ─── Detect OS ───────────────────────────────────────────
ifeq ($(OS),Windows_NT)
    PLATFORM = windows
    EXE = .exe
else
    UNAME := $(shell uname -s)
    ifeq ($(UNAME),Darwin)
        PLATFORM = mac
        EXE =
    else
        PLATFORM = linux
        EXE =
    endif
endif

# ─── Directories ────────────────────────────────────────
OBJDIR = build

# ─── Source files ───────────────────────────────────────
CLIENT_SRC = client/src/client.c
SERVER_SRC = server/src/server.c
NETWORK_SRC = lib/src/network.c
GAME_MAP_SRC = lib/src/game_map.c
PLAYER_MOVEMENT_SRC = lib/src/player_movement.c
LOBBY_SRC = lib/src/lobby.c
TASK_SRC = lib/src/task.c
GAME_SRC = lib/src/game.c
<<<<<<< HEAD
IMPOSTER_ABILITY_SRC = lib/src/imposter_ability.c
=======
SFX_SRC = lib/src/SFX.c
>>>>>>> main

PLAYER_MOVEMENT_SRC = lib/src/player_movement.c
GAME_MAP_SRC = lib/src/game_map.c
NETWORK_SRC = lib/src/network.c

PLAYER_MOVEMENT_TEST_SRC = test_files/test_player_movement.c
GAME_MAP_TEST_SRC = test_files/test_game_map.c

# ─── Object files ───────────────────────────────────────
CLIENT_OBJ = $(OBJDIR)/client.o
SERVER_OBJ = $(OBJDIR)/server.o
NETWORK_OBJ = $(OBJDIR)/network.o
GAME_MAP_OBJ = $(OBJDIR)/game_map.o
PLAYER_MOVEMENT_OBJ = $(OBJDIR)/player_movement.o
LOBBY_OBJ = $(OBJDIR)/lobby.o
TASK_OBJ = $(OBJDIR)/task.o
GAME_OBJ = $(OBJDIR)/game.o
<<<<<<< HEAD
IMPOSTER_ABILITY_OBJ = $(OBJDIR)/imposter_ability.o
=======
SFX_OBJ = $(OBJDIR)/sfx.o
>>>>>>> main

PLAYER_MOVEMENT_OBJ = $(OBJDIR)/player_movement.o
GAME_MAP_OBJ = $(OBJDIR)/game_map.o
NETWORK_OBJ = $(OBJDIR)/network.o

PLAYER_MOVEMENT_TEST_OBJ = $(OBJDIR)/test_player_movement.o
GAME_MAP_TEST_OBJ = $(OBJDIR)/test_game_map.o

# ─── Output files ───────────────────────────────────────
CLIENT_OUT = $(OBJDIR)/client$(EXE)
SERVER_OUT = $(OBJDIR)/server$(EXE)

PLAYER_MOVEMENT_TEST_OUT = $(OBJDIR)/test_player_movement$(EXE)
GAME_MAP_TEST_OUT = $(OBJDIR)/test_game_map$(EXE)

# ─── Compiler ───────────────────────────────────────────
CC = gcc
CFLAGS = -g -Ilib/include
LDFLAGS = -lm

# ─── Platform-specific settings ─────────────────────────
ifeq ($(PLATFORM),mac)
    CC = clang
    CFLAGS += -I/opt/homebrew/include -I/usr/local/include
    LDFLAGS += -L/opt/homebrew/lib -L/usr/local/lib -lSDL2 -lSDL2_image -lSDL2_net -lSDL2_ttf -lSDL2_mixer -lm
endif

ifeq ($(PLATFORM),windows)
    CC = gcc
    CFLAGS += -I/mingw64/include
    LDFLAGS += -L/mingw64/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_net -lSDL2_ttf -lSDL2_mixer -lm
endif

ifeq ($(PLATFORM),linux)
    CC = gcc
    CFLAGS += -I/usr/include
    LDFLAGS += -lSDL2 -lSDL2_image -lSDL2_net -lSDL2_ttf -lSDL2_mixer -lm
endif

# ─── Build folder ───────────────────────────────────────
$(OBJDIR):
ifeq ($(PLATFORM),windows)
	if not exist $(OBJDIR) mkdir $(OBJDIR)
else
	mkdir -p $(OBJDIR)
endif

# ─── Default target ─────────────────────────────────────
all: $(CLIENT_OUT) $(SERVER_OUT)

# ─── Compile rules ──────────────────────────────────────
$(CLIENT_OBJ): $(CLIENT_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_OBJ): $(SERVER_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(PLAYER_MOVEMENT_OBJ): $(PLAYER_MOVEMENT_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(GAME_MAP_OBJ): $(GAME_MAP_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(NETWORK_OBJ): $(NETWORK_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(LOBBY_OBJ): $(LOBBY_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(GAME_OBJ): $(GAME_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(PLAYER_MOVEMENT_TEST_OBJ): $(PLAYER_MOVEMENT_TEST_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(GAME_MAP_TEST_OBJ): $(GAME_MAP_TEST_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TASK_OBJ): $(TASK_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(IMPOSTER_ABILITY_OBJ): $(IMPOSTER_ABILITY_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ─── Link rules ─────────────────────────────────────────
$(CLIENT_OUT): $(CLIENT_OBJ) $(PLAYER_MOVEMENT_OBJ) $(GAME_MAP_OBJ) $(NETWORK_OBJ) $(LOBBY_OBJ) $(GAME_OBJ) $(TASK_OBJ) $(IMPOSTER_ABILITY_OBJ)
$(SFX_OBJ): $(SFX_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ─── Link rules ─────────────────────────────────────────
$(CLIENT_OUT): $(CLIENT_OBJ) $(PLAYER_MOVEMENT_OBJ) $(GAME_MAP_OBJ) $(NETWORK_OBJ) $(LOBBY_OBJ) $(GAME_OBJ) $(TASK_OBJ) $(SFX_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(SERVER_OUT): $(SERVER_OBJ) $(NETWORK_OBJ) $(PLAYER_MOVEMENT_OBJ) $(GAME_MAP_OBJ) $(IMPOSTER_ABILITY_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(PLAYER_MOVEMENT_TEST_OUT): $(PLAYER_MOVEMENT_TEST_OBJ) $(PLAYER_MOVEMENT_OBJ) $(GAME_MAP_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(GAME_MAP_TEST_OUT): $(GAME_MAP_TEST_OBJ) $(GAME_MAP_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

# ─── Run targets ────────────────────────────────────────
run_client: $(CLIENT_OUT)
	./$(CLIENT_OUT)

run_server: $(SERVER_OUT)
	./$(SERVER_OUT)

test_player_movement: $(PLAYER_MOVEMENT_TEST_OUT)
	./$(PLAYER_MOVEMENT_TEST_OUT)

test_game_map: $(GAME_MAP_TEST_OUT)
	./$(GAME_MAP_TEST_OUT)

# ─── Clean ──────────────────────────────────────────────
clean:
ifeq ($(PLATFORM),windows)
	if exist $(OBJDIR) del /Q $(OBJDIR)\*.o $(OBJDIR)\*.exe
else
	rm -rf $(OBJDIR)
endif
