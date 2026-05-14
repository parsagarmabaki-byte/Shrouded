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
IP_CONFIG_SRC = lib/src/ip_config.c
TASK_SRC = lib/src/task.c
TASK_RENDER_SRC = lib/src/task_render.c
TASK_INIT_SRC = lib/src/task_init.c
GAME_SRC = lib/src/game.c
GAME_INPUT_SRC = lib/src/game_input.c
GAME_UPDATE_SRC = lib/src/game_update.c
WALL_DATA_SRC = lib/src/wall_data.c
IMPOSTER_ABILITY_SRC = lib/src/imposter_ability.c
SFX_SRC = lib/src/SFX.c
EMERGENCY_MEETING_SRC = lib/src/emergency_meeting.c
TEXT_SRC = lib/src/text.c
MAIN_MENU_SRC = lib/src/main_menu.c
GAME_RENDER_SRC = lib/src/game_render.c
CLIENT_NETWORK_SRC = client/src/client_network.c
SERVER_BROADCAST_SRC = server/src/server_broadcast.c
SERVER_LOBBY_SRC = server/src/server_lobby.c
SERVER_ROUND_SRC = server/src/server_round.c
SERVER_MEETING_SRC = server/src/server_meeting.c
SERVER_GAME_LOGIC_SRC = server/src/server_game_logic.c

# ─── Object files ───────────────────────────────────────
CLIENT_OBJ = $(OBJDIR)/client.o
SERVER_OBJ = $(OBJDIR)/server.o
NETWORK_OBJ = $(OBJDIR)/network.o
GAME_MAP_OBJ = $(OBJDIR)/game_map.o
PLAYER_MOVEMENT_OBJ = $(OBJDIR)/player_movement.o
LOBBY_OBJ = $(OBJDIR)/lobby.o
IP_CONFIG_OBJ = $(OBJDIR)/ip_config.o
TASK_OBJ = $(OBJDIR)/task.o
TASK_RENDER_OBJ = $(OBJDIR)/task_render.o
TASK_INIT_OBJ = $(OBJDIR)/task_init.o
GAME_OBJ = $(OBJDIR)/game.o
GAME_INPUT_OBJ = $(OBJDIR)/game_input.o
GAME_UPDATE_OBJ = $(OBJDIR)/game_update.o
WALL_DATA_OBJ = $(OBJDIR)/wall_data.o
IMPOSTER_ABILITY_OBJ = $(OBJDIR)/imposter_ability.o
SFX_OBJ = $(OBJDIR)/sfx.o
EMERGENCY_MEETING_OBJ = $(OBJDIR)/emergency_meeting.o
TEXT_OBJ = $(OBJDIR)/text.o
MAIN_MENU_OBJ = $(OBJDIR)/main_menu.o
GAME_RENDER_OBJ = $(OBJDIR)/game_render.o
CLIENT_NETWORK_OBJ = $(OBJDIR)/client_network.o
SERVER_BROADCAST_OBJ = $(OBJDIR)/server_broadcast.o
SERVER_LOBBY_OBJ = $(OBJDIR)/server_lobby.o
SERVER_ROUND_OBJ = $(OBJDIR)/server_round.o
SERVER_MEETING_OBJ = $(OBJDIR)/server_meeting.o
SERVER_GAME_LOGIC_OBJ = $(OBJDIR)/server_game_logic.o

# ─── Output files ───────────────────────────────────────
CLIENT_OUT = $(OBJDIR)/client$(EXE)
SERVER_OUT = $(OBJDIR)/server$(EXE)

# ─── Compiler ───────────────────────────────────────────
CC = gcc
CFLAGS = -g -Ilib/include -Iserver/include
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

$(CLIENT_NETWORK_OBJ): $(CLIENT_NETWORK_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(LOBBY_OBJ): $(LOBBY_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(IP_CONFIG_OBJ): $(IP_CONFIG_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(GAME_OBJ): $(GAME_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(GAME_INPUT_OBJ): $(GAME_INPUT_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(GAME_UPDATE_OBJ): $(GAME_UPDATE_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TASK_OBJ): $(TASK_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TASK_RENDER_OBJ): $(TASK_RENDER_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TASK_INIT_OBJ): $(TASK_INIT_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(WALL_DATA_OBJ): $(WALL_DATA_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(IMPOSTER_ABILITY_OBJ): $(IMPOSTER_ABILITY_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(SFX_OBJ): $(SFX_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(EMERGENCY_MEETING_OBJ): $(EMERGENCY_MEETING_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEXT_OBJ): $(TEXT_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(MAIN_MENU_OBJ): $(MAIN_MENU_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(GAME_RENDER_OBJ): $(GAME_RENDER_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_BROADCAST_OBJ): $(SERVER_BROADCAST_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_LOBBY_OBJ): $(SERVER_LOBBY_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_ROUND_OBJ): $(SERVER_ROUND_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_MEETING_OBJ): $(SERVER_MEETING_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_GAME_LOGIC_OBJ): $(SERVER_GAME_LOGIC_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@


# ─── Link rules ─────────────────────────────────────────
$(CLIENT_OUT): $(CLIENT_OBJ) $(PLAYER_MOVEMENT_OBJ) $(GAME_MAP_OBJ) $(NETWORK_OBJ) $(CLIENT_NETWORK_OBJ) $(LOBBY_OBJ) $(IP_CONFIG_OBJ) $(GAME_OBJ) $(GAME_INPUT_OBJ) $(GAME_UPDATE_OBJ) $(TASK_OBJ) $(SFX_OBJ) $(IMPOSTER_ABILITY_OBJ) $(WALL_DATA_OBJ) $(EMERGENCY_MEETING_OBJ) $(TEXT_OBJ) $(MAIN_MENU_OBJ) $(GAME_RENDER_OBJ) $(TASK_RENDER_OBJ) $(TASK_INIT_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(SERVER_OUT): $(SERVER_OBJ) $(NETWORK_OBJ) $(PLAYER_MOVEMENT_OBJ) $(GAME_MAP_OBJ) $(IMPOSTER_ABILITY_OBJ) $(WALL_DATA_OBJ) $(SERVER_BROADCAST_OBJ) $(SERVER_LOBBY_OBJ) $(SERVER_ROUND_OBJ) $(SERVER_MEETING_OBJ) $(SERVER_GAME_LOGIC_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

# ─── Run targets ────────────────────────────────────────
run_client: $(CLIENT_OUT)
	./$(CLIENT_OUT)

run_server: $(SERVER_OUT)
	./$(SERVER_OUT)


# ─── Clean ──────────────────────────────────────────────
clean:
ifeq ($(PLATFORM),windows)
	if exist $(OBJDIR) del /Q $(OBJDIR)\*.o $(OBJDIR)\*.exe
else
	rm -rf $(OBJDIR)
endif