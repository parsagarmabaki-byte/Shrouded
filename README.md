# Shrouded

An Among Us-inspired multiplayer game built in C with SDL2, developed for CM1008 at KTH.

4–6 players connect over LAN. One player is the **Impostor**, the rest are **Crewmates**. Crewmates complete tasks and try to identify the impostor. The impostor tries to eliminate crewmates without being caught.

---

## Project Structure

```
shrouded/
├── Makefile
├── README.md
├── assets/
│   ├── SFX/
│   ├── fonts/
│   ├── images/
│   ├── lobbyscreen/
│   └── sprites/
├── client/
│   └── src/
│       ├── client.c          ← client entry point: connection prompt, lobby loop, then runGame()
│       └── client_network.c  ← client-side UDP init, join/leave/start/input packets, packet collection
├── server/
│   └── src/
│       └── server.c          ← authoritative game logic, 60fps tick, broadcasts state
├── test_files/
│   ├── test_game_map.c       ← standalone map/camera test
│   └── test_player_movement.c ← standalone movement test
└── lib/
    ├── include/
    │   ├── SFX.h             ← audio loading/playback helpers
    │   ├── client_network.h  ← client networking API used by client/game code
    │   ├── emergency_meeting.h ← emergency meeting UI and request helpers
    │   ├── game.h            ← game loop, client-side prediction, rendering, event handling
    │   ├── game_map.h        ← map assets, rendering, camera follow
    │   ├── imposter_ability.h ← kill/report ability logic and rendering
    │   ├── kill_animation.h  ← kill/body animation data
    │   ├── lobby.h           ← lobby screen and server-address prompt
    │   ├── network.h         ← generic UDP socket/packet helpers
    │   ├── network_data.h    ← shared packet structs (both sides include this)
    │   ├── player_movement.h ← Player struct, movement, animation, renderPlayer()
    │   ├── task.h            ← task ADT and task minigames
    │   ├── text.h            ← reusable text rendering helpers
    │   └── wall_data.h       ← collision/wall-map data
    └── src/
        ├── SFX.c
        ├── emergency_meeting.c
        ├── game.c
        ├── game_map.c
        ├── imposter_ability.c
        ├── lobby.c
        ├── network.c
        ├── player_movement.c
        ├── task.c
        ├── text.c
        └── wall_data.c
```

---

## Architecture

The game uses a **client-server model over UDP** (SDL_net). The server is **authoritative** — it owns all player positions and game state. Clients only send input; they never move players themselves (except for local prediction, see below).

```
Client                              Server
──────                              ──────
SDL_PollEvent()
SDL_GetKeyboardState()
sendInput()
  └─ clientInput ──── UDP ────────► receive clientInput
                                    handleInput()
                                      └─ update player x/y
                                    broadcastGameState()
receive_game_state() ◄── UDP ──────   └─ send gameState to all clients
renderPlayer() for all active players
camera_follow() on local player
```

### server.c

The server runs a single `main()` loop that does two things:

1. **Receives packets** and dispatches by `MessageType`:
   - `MSG_JOIN` → `addToLobby()` assigns a free slot (0–5) and a spawn position
   - `MSG_LEAVE` → `removeFromLobby()` marks the slot as free
   - `MSG_START_GAME` → transitions `gamePhase` from `GAME_LOBBY` to `GAME_RUNNING`
   - `MSG_CLIENT_INPUT` → stores the input in `lastInput[player_id]`

2. **Every 16ms (60fps)**: applies stored input via `handleInput()` (moves `playerState.x/y` by `dx * PLAYER_SPEED * dt`), then calls `broadcastGameState()` which sends the full `gameState` struct to every connected client — with `local_player_id` set correctly per recipient.

The server identifies clients by IP+port. It has no SDL window and no renderer.

### client.c

Entry point. Connects to the server, then runs two sequential phases:

**Lobby phase** (`while state.phase == GAME_LOBBY`):
- Polls SDL events; Space sends `MSG_START_GAME`
- Calls `receive_game_state()` to update the player count
- Calls `renderWaitingScreen()` to draw the lobby UI

**Game phase** (once `state.phase == GAME_RUNNING`):
- Calls `runGame()` from `game.c` and blocks until the player quits

### game.c — runGame() and sendInput()

`runGame()` is the main game loop. Each frame it:

1. Sends keyboard state to the server via `sendInput()`
2. Receives the latest `gameState` from the server (non-blocking — skips if nothing arrived)
3. Applies **client-side prediction**: moves the local player immediately based on current keys, without waiting for the server response. This removes the one-frame input latency the player would otherwise feel.
4. Advances the sprite animation timer
5. Calls `camera_follow()` and `render_map()` to draw the world
6. Loops over all `MAX_PLAYERS` slots — renders local player at predicted position, other players at server-authoritative positions

### network_data.h — shared structs

Both server and client include this file. All communication is done by copying these structs raw into UDP packets:

```c
typedef struct {
    MessageType type;
    int player_id;
    int up, down, left, right;
    int interact, kill, report;
} clientInput;

typedef struct {
    int active;
    int player_id;
    float x, y;
    int isAlive, isImpostor, isDoingTask;
} playerState;

typedef struct {
    MessageType type;
    playerState players[MAX_PLAYERS]; // 6 slots
    gamePhase phase;                  // GAME_LOBBY / GAME_RUNNING / GAME_MEETING
    int local_player_id;
} gameState;
```

The server sets `local_player_id` individually for each recipient before calling `send_game_state()`, so every client knows which slot is theirs.

### player_movement.c — movement and rendering

`move_player()` reads the keyboard, normalises diagonal movement (multiplies by `0.7071` so diagonal speed equals cardinal speed), updates `player.Hitbox.x/y`, and clamps to the map bounds.

`renderPlayer()` clips the correct frame from the sprite sheet. The sheet is a grid of `128×128` px tiles: columns are animation frames (0–3), rows are directions (`DIR_DOWN=0`, `DIR_LEFT=1`, `DIR_RIGHT=2`, `DIR_UP=3`).

### game_map.c — camera and map

`camera_follow()` centers the camera on the player by computing `cam.x = player_x + player_w/2 - screen_w/2`, then clamps to the map edges so the camera never shows outside the `3072×2048` map.

`render_map()` draws the full map texture offset by `-cam.x / -cam.y`, so only the visible portion is shown on screen.

### lobby.c — SDL initialisation and waiting screen

`initiate()` initialises SDL2, SDL_ttf, SDL_image, creates the window and renderer, loads the font and lobby background texture. `renderWaitingScreen()` draws the player count (`X/6 CONNECTED`) and, once at least one player is connected, shows "PRESS SPACE TO START".

---

## Data Flow Summary

```
network_data.h          (shared structs — included by everyone)
      │
      ├── server.c      main() → handleInput() → broadcastGameState()
      │       │
      │   network.c     init_server(), receive_client_input(), send_game_state()
      │
      └── client.c      main() → lobby loop → runGame()
              │
          game.c        runGame(), sendInput()
          game_map.c    loading_img(), render_map(), camera_follow()
          player_movement.c  init_player(), move_player(), renderPlayer()
          lobby.c       initiate(), renderWaitingScreen(), cleanLobby()
          network.c     init_client(), send_join(), receive_game_state(), send_client_input()
```

---

## Requirements

### Mac (Apple Silicon)

```bash
brew install sdl2 sdl2_image sdl2_net sdl2_ttf
```

### Windows (MSYS2/MinGW64)

```bash
pacman -S mingw64/mingw-w64-x86_64-SDL2 \
          mingw64/mingw-w64-x86_64-SDL2_image \
          mingw64/mingw-w64-x86_64-SDL2_net \
          mingw64/mingw-w64-x86_64-SDL2_ttf
```

---

## Build & Run

```bash
make all                   # build server and client → build/server, build/client
make clean                 # remove build/ directory
make test_player_movement  # standalone movement test (no network required)
```

### Starting a game

```bash
# Terminal 1 — start the server
./build/server

# Terminal 2 (and more) — start a client
./build/client
```

Once clients are connected, press **Space** to start the game.

> **Note:** The client currently connects to `127.0.0.1` (localhost) by default — see `init_client()` in `network.c`. For real LAN play this must be changed to the host's IP address.

---

## Controls

| Key | Action |
|---|---|
| `W A S D` | Move |
| `Space` | Start game (lobby only) |
| `Escape` | Quit |

---

## Tech Stack

| | |
|---|---|
| Language | C |
| Graphics | SDL2, SDL2_image, SDL2_ttf |
| Networking | SDL_net (UDP) |
| Platforms | Mac (Homebrew, Apple Silicon), Windows (MSYS2/MinGW64) |
| Build system | Makefile (auto-detects OS) |
| Version control | GitHub |
