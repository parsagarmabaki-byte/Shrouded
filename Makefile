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

CLIENT_OBJ = $(OBJDIR)/client.o
SERVER_OBJ = $(OBJDIR)/server.o
NETWORK_OBJ = $(OBJDIR)/network.o

CFLAGS  = -g -c -Ilib/include
LDFLAGS = -lSDL2main -lSDL2 -lSDL2_image -lSDL2_net -lm -lSDL2_ttf -lm

SERVER_OUT = build/server
CLIENT_OUT = build/client

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

$(NETWORK_OBJ): $(NETWORK_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) $(NETWORK_SRC) -o $(NETWORK_OBJ)

$(CLIENT_OUT): $(CLIENT_OBJ) $(NETWORK_OBJ)
	$(CC) $(CLIENT_OBJ) $(NETWORK_OBJ) -o $(CLIENT_OUT) $(LDFLAGS)

$(SERVER_OUT): $(SERVER_OBJ) $(NETWORK_OBJ)
	$(CC) $(SERVER_OBJ) $(NETWORK_OBJ) -o $(SERVER_OUT) $(LDFLAGS)

run: $(CLIENT_OUT)
	./$(CLIENT_OUT)

clean:
ifeq ($(PLATFORM),windows)
	del /Q $(CLIENT_OUT) $(SERVER_OUT)
	rmdir /S /Q $(OBJDIR)
else
	rm -rf $(OBJDIR) $(CLIENT_OUT) $(SERVER_OUT)
endif