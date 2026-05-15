# Shrouded

An Among Us-inspired multiplayer game built in C with SDL2 for CM1008 at KTH.

Players connect over LAN. One player is the impostor and the rest are crewmates. Crewmates complete tasks, report bodies, and call meetings. The impostor tries to eliminate crewmates without being caught.

---

## Current Status

Implemented:

- UDP client-server networking with SDL_net.
- Server-authoritative player slots, positions, alive state, impostor role, kill cooldowns, and game phases.
- Lobby screen with server IP prompt.
- Role reveal screen.
- Player movement, camera follow, wall collision, sprite animation, and map rendering.
- Kill request flow, body animations, body reporting, emergency meetings, and meeting UI.
- Local task minigames: timer, click, typing, reflex, logical order, and memory.
- Task map overlay with example task markers and local player position.
- Lobby music and SDL_mixer audio setup.

Partially implemented / future work:

- Win phase enum values exist (`GAME_CREWMATES_WIN`, `GAME_KILLER_WIN`), and `check_win_condition()` exists on the server, but automatic win transitions are not fully wired into the game flow yet.
- Task progress is currently local to the client task ADT. Server-authoritative task progress and task-based win conditions still need to be added.
- Voting/ejection logic is not implemented yet.

---

## Project Structure

```text
shrouded/
|-- Makefile
|-- README.md
|-- assets/
|   |-- SFX/
|   |   `-- lobby_music.mp3
|   |-- fonts/
|   |-- images/
|   |-- lobbyscreen/
|   `-- sprites/
|-- client/
|   `-- src/
|       |-- client.c          # client entry point, IP prompt, lobby loop, game handoff
|       `-- client_network.c  # client UDP init, join/leave/start/input/report packets
|-- server/
|   `-- src/
|       `-- server.c          # authoritative game state, packet handling, 60fps tick
|-- test_files/
|   |-- test_game_map.c
|   `-- test_player_movement.c
`-- lib/
    |-- include/
    |   |-- SFX.h
    |   |-- client_network.h
    |   |-- emergency_meeting.h
    |   |-- game.h
    |   |-- game_map.h
    |   |-- imposter_ability.h
    |   |-- kill_animation.h
    |   |-- lobby.h
    |   |-- network.h
    |   |-- network_data.h
    |   |-- player_movement.h
    |   |-- task.h
    |   |-- text.h
    |   `-- wall_data.h
    `-- src/
        |-- SFX.c
        |-- emergency_meeting.c
        |-- game.c
        |-- game_map.c
        |-- imposter_ability.c
        |-- lobby.c
        |-- network.c
        |-- player_movement.c
        |-- task.c
        |-- text.c
        `-- wall_data.c
```

---

## Architecture

The game uses a client-server model over UDP with SDL_net.

The server is authoritative. It owns the shared `gameState`, including:

- connected player slots
- player positions
- alive/dead state
- impostor assignment
- kill cooldowns
- game phase
- emergency meeting/report state

Clients send requests and input. They render locally from the latest state received from the server.

```text
Client                                  Server
------                                  ------
promptServerAddress()
init_client()
send_join()        ---- MSG_JOIN ---->  addToLobby()

Lobby:
SDL_PollEvent()
send_start_game()  - MSG_START_GAME ->  designateImpostor()
                                          phase = GAME_SHOW_ROLE

Game:
read_input()
send_input()       - MSG_CLIENT_INPUT > store latest input
                                          handleInput()
                                          apply_movement()
collect_packets()  <--- gameState ----  broadcastGameState()
render_game()
```

Networking is split into two layers:

- `lib/src/network.c`: generic UDP helpers (`init_network_socket`, `create_packet`, `send_packet_data`, `packet_has_size`).
- `client/src/client_network.c`: client-specific messages (`init_client`, `send_join`, `send_start_game`, `send_input`, `request_kill`, `request_report_body`, `request_emergency_meeting`, `collect_packets`).

---

## Game Phases

The shared `gamePhase` enum is defined in `lib/include/network_data.h`:

```c
typedef enum {
    GAME_LOBBY,
    GAME_RUNNING,
    GAME_SHOW_ROLE,
    GAME_INFO_MEETING,
    GAME_MEETING,
    GAME_CREWMATES_WIN,
    GAME_KILLER_WIN
} gamePhase;
```

Current phase flow:

```text
GAME_LOBBY
  -> GAME_SHOW_ROLE
  -> GAME_RUNNING
  -> GAME_INFO_MEETING
  -> GAME_MEETING
  -> GAME_RUNNING
```

`GAME_CREWMATES_WIN` and `GAME_KILLER_WIN` exist, but final win-screen behavior and task-win logic are still incomplete.

---

## Shared Network Data

Both client and server include `lib/include/network_data.h`. UDP packets are sent by copying C structs into packet data.

Important shared types:

```c
typedef enum {
    MSG_JOIN,
    MSG_LEAVE,
    MSG_START_GAME,
    MSG_CLIENT_INPUT,
    MSG_GAME_STATE,
    MSG_READY_STATUS,
    MSG_KILL_REQUEST,
    MSG_KILL_EVENT,
    MSG_EMERGENCY_MEETING,
    MSG_BODY_FOUND
} MessageType;
```

```c
typedef struct {
    MessageType type;
    int player_id;
    int up, down, left, right;
    int interact, kill, report;
    int current_frame;
    int isAlive;
    int emergency_meeting_left;
    Direction direction;
} clientInput;
```

```c
typedef struct {
    int active;
    int player_id;
    float x, y;
    int isAlive;
    int isImpostor;
    int isDoingTask;
    bool kill_cooldown_active;
    Uint32 kill_cooldown_start;
    int emergency_meeting;
    int current_frame;
    Direction direction;
} playerState;
```

```c
typedef struct {
    MessageType type;
    playerState players[MAX_PLAYERS];
    gamePhase phase;
    int local_player_id;
    int emergency_meeting_reported_id;
} gameState;
```

Note: because structs are copied directly into UDP packets, this setup assumes compatible builds on the machines involved.

---

## Main Modules

### `server/src/server.c`

Runs the authoritative server loop.

Receives:

- `MSG_JOIN`
- `MSG_LEAVE`
- `MSG_START_GAME`
- `MSG_CLIENT_INPUT`
- `MSG_KILL_REQUEST`
- `MSG_EMERGENCY_MEETING`
- `MSG_BODY_FOUND`

Every server tick, it applies stored input, updates cooldowns, advances phase timers, and broadcasts `gameState` to connected clients.

### `client/src/client.c`

Owns the high-level client flow:

1. Initializes the lobby window.
2. Prompts for server IP.
3. Calls `init_client()`.
4. Sends `MSG_JOIN`.
5. Runs the lobby loop.
6. Starts `runGame()` when the server leaves `GAME_LOBBY`.

It no longer contains low-level socket setup or packet creation.

### `client/src/client_network.c`

Owns client-side networking:

- opens the UDP socket
- resolves the server address
- allocates the receive packet
- sends join/start/leave/input/kill/report/emergency requests
- collects incoming `gameState` and kill-event packets

### `lib/src/game.c`

Runs the main client-side game loop:

1. Handles special phases such as role reveal and meetings.
2. Processes SDL events.
3. Reads movement input.
4. Applies local prediction.
5. Sends input to the server.
6. Receives server packets.
7. Reconciles local position with the server position.
8. Updates tasks and kill animations.
9. Renders the map, players, bodies, buttons, tasks, emergency UI, and task map.

### `lib/src/player_movement.c`

Contains:

- `player_create()` / `player_destroy()`
- `apply_movement()`
- server-position reconciliation
- player sprite rendering

Movement is normalized diagonally and blocked by `wall_data.c` collision checks.

### `lib/src/game_map.c`

Loads game textures and renders the map. It also provides camera following and cleanup for loaded assets.

Current map constants:

```c
#define GAME_MAP_WIDTH 2536
#define GAME_MAP_HEIGHT 2024
#define LOGICAL_SCREEN_WIDTH 1280
#define LOGICAL_SCREEN_HEIGHT 720
```

### `lib/src/task.c`

Task ADT and local task minigames:

- `TASK_TIMER`
- `TASK_CLICK`
- `TASK_TYPE`
- `TASK_REFLEX`
- `TASK_LOGICAL_ORDER`
- `TASK_MEMORY`

Task completion currently increments local task points. Server-side task progress is not implemented yet.

### `lib/src/imposter_ability.c`

Contains impostor and body-report logic:

- kill target detection
- kill cooldown updates
- kill animation state
- report-button state
- body target detection

### `lib/src/emergency_meeting.c`

Contains emergency meeting UI rendering helpers.

### `lib/src/SFX.c`

Initializes SDL_mixer, loads music, starts/stops lobby music, and cleans audio assets.

---

## Client Game Loop Summary

```text
runGame()
  handle_game_phase()
  process_events()
    task_events()
    kill_events()
    emergency_meeting_events()
    report_body_events()
  update_player_movement()
  send_player_input()
  collect_packets()
  compare_server_position()
  update_task()
  update_kill_animation()
  render_game()
```

---

## Server Tick Summary

```text
server main loop
  receive packet
    MSG_JOIN              -> addToLobby()
    MSG_LEAVE             -> removeFromLobby()
    MSG_START_GAME        -> designateImpostor(), GAME_SHOW_ROLE
    MSG_CLIENT_INPUT      -> store latest input for sender
    MSG_KILL_REQUEST      -> handle_kill_request()
    MSG_EMERGENCY_MEETING -> GAME_INFO_MEETING
    MSG_BODY_FOUND        -> GAME_INFO_MEETING

  every SERVER_TICK_INTERVAL
    GAME_SHOW_ROLE    -> after delay, GAME_RUNNING
    GAME_INFO_MEETING -> after delay, GAME_MEETING
    GAME_MEETING      -> after delay, respawn/reposition, GAME_RUNNING
    GAME_RUNNING      -> apply input, update cooldowns, broadcast state
```

---

## Requirements

### macOS

```bash
brew install sdl2 sdl2_image sdl2_net sdl2_ttf sdl2_mixer
```

The Makefile checks both `/opt/homebrew` and `/usr/local`.

### Windows (MSYS2/MinGW64)

```bash
pacman -S mingw64/mingw-w64-x86_64-SDL2 \
          mingw64/mingw-w64-x86_64-SDL2_image \
          mingw64/mingw-w64-x86_64-SDL2_net \
          mingw64/mingw-w64-x86_64-SDL2_ttf \
          mingw64/mingw-w64-x86_64-SDL2_mixer
```

### Linux

Install SDL2, SDL2_image, SDL2_net, SDL2_ttf, and SDL2_mixer through your distribution package manager.

---

## Build

```bash
make all
```

Outputs:

```text
build/client
build/server
```

Other targets:

```bash
make run_server
make run_client
make test_player_movement
make test_game_map
make clean
```

---

## Run

Start the server first:

```bash
./build/server
```

Then start one or more clients:

```bash
./build/client
```

The client prompts for the server IP address. For local testing on one computer, use:

```text
127.0.0.1
```

For LAN play, use the host computer's local network IP, for example:

```text
192.168.1.42
```

All players must be on the same network unless additional port-forwarding/VPN setup is used. The server listens on UDP port `2000`.

---

## Controls

| Key / Input | Action |
|---|---|
| `W A S D` | Move |
| `Space` | Start game in lobby |
| `Escape` | Quit game, or close emergency window if it is open |
| `M` | Toggle task map overlay |
| `E` | Open emergency button view when near the emergency table |
| Mouse click on emergency button | Request emergency meeting |
| `K` | Kill as impostor when target/cooldown allow it |
| Mouse click on kill button | Kill as impostor when target/cooldown allow it |
| `R` | Report nearby body |
| Mouse click on report button | Report nearby body |
| `1` | Start timer task |
| `2` | Start click task |
| `3` | Start typing task |
| `4` | Start reflex task |
| `5` | Start logical order task |
| `6` | Start memory task |
| `Q` | Cancel active task |
| Mouse click | Task interaction for click/logical-order tasks |

---

## Task Map

The task map overlay is rendered in `render_task_map()` in `lib/src/game.c`.

It currently:

- draws a dark overlay
- draws the real `Game_map.png` scaled into the overlay
- draws example task markers
- draws the local player's position as a blue marker

Example markers are currently hardcoded and should eventually be replaced by task data from a server-authoritative task system.

---

## Notes

- The server has no SDL window. It only runs networking and authoritative game state.
- The client performs local movement prediction, then corrects against the server position with `compare_server_position()`.
- The lobby currently allows starting with at least one connected player, which is useful for testing.
- `MAX_PLAYERS` is `6`.
- `SERVER_TICK_INTERVAL` is `0.016f`, approximately 60 ticks per second.
- Task progress should be moved to the server before it is used for real win conditions.

---

## Tech Stack

| Area | Technology |
|---|---|
| Language | C |
| Graphics | SDL2, SDL2_image |
| Text | SDL2_ttf |
| Audio | SDL2_mixer |
| Networking | SDL_net over UDP |
| Build system | Make |
| Supported build targets | macOS, Windows/MSYS2, Linux |
