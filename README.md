# Shrouded

A LAN-based multiplayer impostor game written in C using SDL2, developed for CM1008 at KTH.

Players connect over UDP. One player is the impostor and the rest are crewmates. Crewmates complete tasks, report bodies, call meetings, and vote. The impostor tries to eliminate crewmates without being caught.

---

## Current Status

Implemented:

- UDP client-server architecture with SDL_net.
- Server-authoritative game state: player slots, positions, alive/dead state, impostor role, kill cooldowns, and game phase transitions.
- Lobby with server IP prompt and start-game flow.
- Role reveal screen and player sprite animation.
- Movement, camera follow, wall collision, and map rendering.
- Kill request flow, body report detection, emergency meetings, voting, and vote-result display.
- Local task minigames: timer, click, typing, reflex, logical order, and memory.
- Task progress is tracked in server state and contributes to crew win conditions.
- SDL_mixer audio initialization and lobby music playback.

Work-in-progress:

- UI polish and additional in-game feedback.
- More map and asset coverage.
- Improved network robustness and resynchronization.

---

## Repository Structure

```text
.
|-- Makefile
|-- README.md
|-- assets/
|-- build/
|-- client/
|   `-- src/
|-- lib/
|   |-- include/
|   `-- src/
|-- server/
|   |-- include/
|   `-- src/
|-- Fonts/
`
```

- `client/src/`: client entrypoint, lobby flow, network client, and input handling.
- `server/src/`: authoritative server loop, packet handling, meeting/voting logic, and win-condition management.
- `lib/include/`: shared type definitions, network data, and public interfaces.
- `lib/src/`: shared game systems such as rendering, input, tasks, movement, and audio.

---

## Architecture

The game uses a client-server model over UDP with SDL_net.

The server is authoritative for:

- shared `gameState`
- player positions and alive state
- impostor assignment
- task completion and win conditions
- emergency meetings and voting

Clients send requests and input, then render locally from the latest server state.

Shared network definitions are in `lib/include/network_data.h`.

---

## Game Phases

The shared `gamePhase` enum includes:

```c
GAME_LOBBY
GAME_RUNNING
GAME_SHOW_ROLE
GAME_INFO_MEETING
GAME_MEETING
SHOW_VOTE_RESULT
GAME_CREWMATES_WIN
GAME_KILLER_WIN
```

Typical flow:

```text
GAME_LOBBY
  -> GAME_SHOW_ROLE
  -> GAME_RUNNING
  -> GAME_INFO_MEETING
  -> GAME_MEETING
  -> SHOW_VOTE_RESULT
  -> GAME_RUNNING
```

Win phases are used for crew/impostor victory display.

---

## Networking

Both client and server use `lib/include/network_data.h` for shared packet formats. UDP packets are sent by copying structs into packet data.

Important shared packet types include:

- `MSG_JOIN`
- `MSG_LEAVE`
- `MSG_START_GAME`
- `MSG_CLIENT_INPUT`
- `MSG_GAME_STATE`
- `MSG_KILL_REQUEST`
- `MSG_KILL_EVENT`
- `MSG_EMERGENCY_MEETING`
- `MSG_BODY_FOUND`
- `MSG_TASK_COMPLETE`
- `MSG_VOTE_REQUEST`

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

Other useful targets:

```bash
make run_server
make run_client
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

The client prompts for the server IP address. For local testing, use `127.0.0.1`.

For LAN play, use the host machine's local address.

---

## Controls

| Key / Input | Action |
|---|---|
| `W A S D` | Move |
| `Space` | Start game in lobby |
| `Escape` | Quit or close emergency/vote screens |
| `M` | Toggle task map overlay |
| `E` | Open emergency meeting view when near the table |
| Mouse click on emergency button | Request emergency meeting |
| `K` | Kill as impostor when allowed |
| Mouse click on kill button | Kill as impostor when allowed |
| `R` | Report nearby body |
| Mouse click on report button | Report nearby body |
| `1` | Start timer task |
| `2` | Start click task |
| `3` | Start typing task |
| `4` | Start reflex task |
| `5` | Start logical order task |
| `6` | Start memory task |
| `Q` | Cancel active task |
| Mouse click | Interact with task UI |

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
| Supported platforms | macOS, Windows/MSYS2, Linux |
`
