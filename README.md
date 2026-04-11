# Shrouded

An Among Us-inspired multiplayer game built in C with SDL2, developed for CM1008 at KTH.

4–6 players connect over LAN. One player is the **Impostor**, the rest are **Crewmates**. Crewmates complete tasks and try to identify the impostor. The impostor tries to eliminate crewmates without being caught.

---

## Project Structure

```
shrouded/
├── assets/
│   ├── fonts/
│   ├── images/
│   ├── lobbyscreen/
│   └── sprites/
├── client/
│   └── src/client.c          ← lobby loop, connects to server
├── server/
│   └── src/server.c          ← game tick, player positions, broadcast
├── test_files/
│   └── test_player_movement.c ← standalone movement test (no network)
└── lib/
    ├── include/
    │   ├── game.h            ← game loop, rendering
    │   ├── game_map.h        ← map, camera
    │   ├── lobby.h           ← lobby screen
    │   ├── network.h         ← send/receive wrappers
    │   ├── network_data.h    ← shared packet structs
    │   └── player_movement.h ← player struct, animation, movement
    └── src/
        ├── game.c
        ├── game_map.c
        ├── lobby.c
        ├── network.c
        └── player_movement.c
```

### Modules

| File | Responsibility |
|---|---|
| `game.h/c` | Game loop, input, rendering all players |
| `game_map.h/c` | Map rendering, camera follow |
| `lobby.h/c` | Lobby screen, waiting for players |
| `network.h/c` | UDP send/receive wrappers |
| `network_data.h` | Shared packet structs (clientInput, gameState, playerState) |
| `player_movement.h/c` | Player struct, sprite animation, renderPlayer |

---

## Architecture

Client-server over UDP (SDL_net). The server is authoritative — clients send input, the server updates all player positions and broadcasts the full game state to every client at 60fps.

```
Client                        Server
──────                        ──────
SDL_PollEvent()
SDL_GetKeyboardState()
send clientInput ──UDP──────► receive clientInput
                              update player position
receive gameState ◄──UDP───── broadcast gameState to all
renderPlayer() for each
camera_follow() local player
```

---

## Requirements

### Mac (Apple Silicon)

```bash
brew install sdl2 sdl2_image sdl2_net sdl2_ttf
```

### Windows (MSYS2/MinGW64)

```bash
pacman -S mingw64/mingw-w64-x86_64-SDL2 mingw64/mingw-w64-x86_64-SDL2_image mingw64/mingw-w64-x86_64-SDL2_net mingw64/mingw-w64-x86_64-SDL2_ttf
```

---

## Build & Run

```bash
make all                  # build server and client
make clean                # remove build files
make test_player_movement # run movement test without network
```

### Start the game

```bash
./build/server   # terminal 1 — start server
./build/client   # terminal 2 — start client
```

When all players are connected, press **Space** to start the game.

> **Note:** The client currently connects to `127.0.0.1` by default. LAN support with custom IP is coming soon.

---

## Controls

| Key | Action |
|---|---|
| `W A S D` | Move |
| `Space` | Start game (in lobby) |
| `Escape` | Quit |

---

## Tech Stack

- **Language:** C
- **Graphics:** SDL2, SDL2_image, SDL2_ttf
- **Networking:** SDL_net (UDP)
- **Platforms:** Mac (Homebrew, Apple Silicon), Windows (MSYS2/MinGW64)
- **Build system:** Makefile
- **Version control:** GitHub