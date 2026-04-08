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
PLAYER_MOVEMENT_SRC = lib/src/player_movement.c

CLIENT_OBJ = $(OBJDIR)/client.o
SERVER_OBJ = $(OBJDIR)/server.o
PLAYER_MOVEMENT_OBJ = $(OBJDIR)/player_movement.o

CFLAGS  = -g -c -Ilib/include
LDFLAGS = -lSDL2main -lSDL2 -lSDL2_image -lSDL2_net -lm -lSDL2_ttf -lm

SERVER_OUT = build/server
CLIENT_OUT = build/client
TEST_PM_OUT = build/player_movement

# ─── Per plattform ───────────────────────────────────────
ifeq ($(PLATFORM),mac)
    CC      = gcc
    CFLAGS += -I/opt/homebrew/include
    LDFLAGS += -L/opt/homebrew/lib

else ifeq ($(PLATFORM),linux)
    CC      = gcc
    CFLAGS += -I/usr/include

else ifeq ($(PLATFORM),windows)
    CC       = gcc
    CFLAGS  += -I/mingw64/include
    LDFLAGS += -L/mingw64/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_net -lm -lSDL2_ttf -lm
endif

# ─── Bygg-regler ─────────────────────────────────────────
$(OBJDIR):
	mkdir -p $(OBJDIR)

all: $(SERVER_OUT) $(CLIENT_OUT)

$(CLIENT_OBJ): $(CLIENT_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_OBJ)

$(SERVER_OBJ): $(SERVER_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_OBJ)

$(TEST_PM_OBJ): $(TEST_PM_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) $(TEST_PM_SRC) -o $(TEST_PM_OBJ)

$(PLAYER_MOVEMENT_OBJ): $(PLAYER_MOVEMENT_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) $(PLAYER_MOVEMENT_SRC) -o $(PLAYER_MOVEMENT_OBJ)

$(CLIENT_OUT): $(CLIENT_OBJ)
	$(CC) $(CLIENT_OBJ) -o $(CLIENT_OUT) $(LDFLAGS)

$(SERVER_OUT): $(SERVER_OBJ)
	$(CC) $(SERVER_OBJ) -o $(SERVER_OUT) $(LDFLAGS)

$(TEST_PM_OUT): $(TEST_PM_OBJ) $(PLAYER_MOVEMENT_OBJ)
	$(CC) $(TEST_PM_OBJ) $(PLAYER_MOVEMENT_OBJ) -o $(TEST_PM_OUT) $(LDFLAGS)

build_test_player_movement: $(TEST_PM_OUT)

player_movement: $(TEST_PM_OUT)
	./$(TEST_PM_OUT)

run: $(CLIENT_OUT)
	./$(CLIENT_OUT)

clean:
ifeq ($(PLATFORM),windows)
	del /Q $(CLIENT_OUT) $(SERVER_OUT)
	rmdir /S /Q $(OBJDIR)
else
	rm -rf $(OBJDIR) $(CLIENT_OUT) $(SERVER_OUT) $(TEST_PM_OUT)
endif