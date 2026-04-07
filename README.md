# Shrouded

An Among Us-inspired multiplayer game built in C with SDL2, developed for CM1008 at KTH.

4–6 players connect over LAN. One player is the **Impostor**, the rest are **Crewmates**. Crewmates complete tasks and try to identify the impostor. The impostor tries to eliminate crewmates without being caught.

---

## Project Structure

```
shrouded/
├── assets/
│   ├── sprites/
│   └── maps/
├── client/
│   └── src/client.c          ← SDL2 loop, input, rendering
├── server/
│   └── src/server.c          ← server loop, connections, tick logic
└── lib/
    ├── include/
    │   ├── player.h
    │   ├── map.h
    │   ├── task.h
    │   ├── game.h
    │   └── network.h        ← shared packet structs
    └── src/
        ├── player.c
        ├── map.c
        ├── task.c
        └── game.c
```

### Modules (ADTs)

| File | Responsibility |
|---|---|
| `player.h/c` | Player data: position, role, state |
| `map.h/c` | Map data, collision, rooms |
| `task.h/c` | Task positions and completion status |
| `game.h/c` | Game logic: win conditions, phases (lobby/game/vote) |
| `network.h/c` | Packet structs and send/receive wrappers |

---

## Requirements

### Mac (Apple Silicon)

```bash
brew install sdl2 sdl2_image sdl2_net
```

### Windows (MSYS2/MinGW64)

```bash
pacman -S mingw64/mingw-w64-x86_64-SDL2 mingw64/mingw-w64-x86_64-SDL2_image mingw64/mingw-w64-x86_64-SDL2_net
```

---

## Build & Run

```bash
make all      # build all files
make clean    # remove build files
```

### Start the game

**Step 1 — Host starts the server:**
```bash
./build/server # Start the server
```

**Step 2 — All players start the client:**
```bash
./build/client # Start one or more clients
```
**Step 3 — Multiplayer (LAN)**
```bash

./build/client 127.0.0.1      # if you are the host
./build/client 192.168.1.5    # other players (host's local IP)

```
Find the host's IP:
```bash
ifconfig   # Mac/Linux
ipconfig   # Windows
```

---

## Tech Stack

- **Language:** C
- **Graphics:** SDL2 (v2.32.10)
- **Networking:** SDL_net (UDP)
- **Platforms:** Mac (Homebrew, Apple Silicon), Windows (MSYS2/MinGW64)
- **Build system:** Makefile
- **Version control:** GitHub