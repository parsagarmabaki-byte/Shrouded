# Shrouded

Shrouded is a LAN-based multiplayer social deduction game inspired by *Among
Us*. It is written in C with SDL2 and was developed for the CM1008 project
course at KTH.

One player is secretly the **Killer**; the rest are **Innocents**. Innocents win
by completing all of their tasks or voting the Killer out at a meeting; the
Killer wins by eliminating Innocents until they are equal to or outnumber the
remaining Innocents.

This document is aimed at developers working *on* the codebase. For player and
host instructions, see the release README.

---

## Tech stack

| Area              | Technology              |
| ----------------- | ----------------------- |
| Language          | C                       |
| Graphics          | SDL2, SDL2_image        |
| Text rendering    | SDL2_ttf                |
| Audio             | SDL2_mixer              |
| Networking        | SDL2_net (UDP **and** TCP) |
| Build system      | Make                    |
| Platforms         | Windows, Linux, macOS   |

---

## Repository structure

```text
.
├── Makefile
├── README.md
├── assets/                 game images, fonts (assets/fonts), and SFX (assets/SFX)
├── client/
│   └── src/
│       ├── client.c            outer loop: menu → lobby → game, connection setup
│       └── client_network.c    UDP send + TCP receive/demux on the client
├── server/
│   ├── include/
│   │   └── server_context.h    the central `Server` struct (all server state)
│   └── src/
│       ├── server.c            main loop, server tick, all message handlers
│       ├── server_broadcast.c  UDP/TCP broadcast helpers
│       ├── server_lobby.c      join/leave, spawn_players, count_active_players
│       ├── server_round.c      start_new_round, impostor + task assignment
│       ├── server_meeting.c    voting, vote tally, TCP vote connections
│       └── server_game_logic.c win condition, input application
└── lib/
    ├── include/                shared headers (public interfaces)
    │   ├── network_data.h       ALL packet structs, enums, and tuning constants
    │   ├── game.h               Client + GameContext structs
    │   └── ...
    └── src/                    shared gameplay systems
        ├── game.c               runGame loop, GameContext lifecycle
        ├── game_input.c         keyboard/mouse handling per phase
        ├── game_update.c        client-side movement prediction
        ├── game_render.c        all in-game rendering + phase dispatch
        ├── killer_ability.c     kill targeting, cooldown, body reporting
        ├── emergency_meeting.c  meeting UI, banners, voting interaction
        ├── lobby.c / main_menu.c lobby + menu screens
        ├── task*.c              task minigames
        └── network.c            low-level socket/packet helpers
        └── ...
```

`lib/include/network_data.h` is the single source of truth for the wire format
and is shared by both binaries — start there.

---

## Architecture

Shrouded is **server-authoritative**. The server owns and validates:

- player positions, alive/dead status, and role assignment
- kill eligibility (requester must be alive, active, and the impostor; the
  server independently recomputes the target and rejects mismatches)
- dead-body positions (clients send only a `body_id`, never coordinates)
- vote eligibility, vote tally, and meeting/phase transitions
- task progress and win conditions

Clients send input and requests, and render the most recent synchronized state.
This prevents a client from directly setting important game state, which closes
several basic cheat vectors.

### Hybrid UDP + TCP networking

This is the defining feature of the project. The server opens **two** ports:

- `SERVER_PORT` (2000) — UDP
- `SERVER_TCP_PORT` (2001) — TCP

Each client opens a UDP socket and a single TCP connection on startup.

| Transport | Carries | Frequency | Why |
| --------- | ------- | --------- | --- |
| **UDP** | `MSG_PLAYER_SYNC_DATA`, `MSG_CLIENT_INPUT`, `MSG_GAME_STATE`, `MSG_MEETING_TIMER`, joins/leaves | every tick (~60/s) | low latency; a dropped packet is harmless because the next tick overwrites it |
| **TCP** | `MSG_KILL_EVENT`, `MSG_EMERGENCY_MEETING`, `MSG_BODY_FOUND`, `MSG_PHASE_CHANGE`, `MSG_TASK_COMPLETE`, `MSG_VOTE_REQUEST`, `MSG_VOTE_UPDATE`, `MSG_MEETING_ENDED`, `MSG_KILL_READY` | on-event (rare) | must arrive exactly once and in order; losing one permanently diverges game state |

Because TCP is a byte stream (a single send may arrive split across several
`recv` calls), both sides use **partial-read buffering**: a byte counter
accumulates into a buffer and the message is only processed once the full struct
has arrived. The server reads incoming `VoteRequest`s per connection using
`voteBytesRead[]` / `voteBuffers[]` (in the `Server` struct). The client uses a
single `tcp_buffer` union (`VoteUpdateMsg` / `PhaseChangeMsg`) and one
`tcp_bytes_read` counter (in the `Client` struct): it first reads the 4-byte
`MessageType` header, then reads the rest of the message sized to that type, and
dispatches on the header — so the read framing never depends on the current game
phase.

> All TCP messages on the wire are either a `PhaseChangeMsg` (used for every
> critical event type) or a `VoteUpdateMsg`. The first 4 bytes are always the
> `MessageType`, which is how the receiver tells them apart.

---

## Game state flow

The shared `GamePhase` enum drives the state machine on both sides:

```c
GAME_LOBBY → GAME_SHOW_ROLE → GAME_RUNNING ⇄ GAME_INFO_MEETING → GAME_MEETING
           → SHOW_VOTE_RESULT → GAME_RUNNING → … → GAME_INNOCENTS_WIN | GAME_KILLER_WIN
```

The server advances phases in `update_server_tick()` (server.c) using timers in
the `Server` struct (`state_start_time`, `phase_time`). Phase changes are pushed
to clients over TCP as `PhaseChangeMsg`. "Play again" sends `MSG_PLAY_AGAIN`,
which calls `start_new_round()` and returns the state machine to
`GAME_SHOW_ROLE` without tearing down connections.

---

## Key constants (lib/include/network_data.h)

```c
MAX_PLAYERS            6        // hard cap; many arrays are sized to this
SERVER_PORT            2000     // UDP
SERVER_TCP_PORT        2001     // SERVER_PORT + 1
TASK_COUNT             8        // tasks assigned per innocent
SERVER_TICK_INTERVAL   0.016f   // ~60 ticks/second
MEETING_DURATION       120000   // ms — voting window (2 minutes)
VOTE_RESULT_DURATION   10000    // ms — result screen
INFO_MEETING_DURATION  3000     // ms — pre-vote info screen
SHOW_ROLE_DURATION     6000     // ms — role reveal
```

```c
COOLDOWN     30000   // ms — kill cooldown (lib/include/killer_ability.h)
KILL_RADIUS  100     // px — kill/report range
```

Minimum players to start and other gameplay rules are enforced in
`handle_start_game()` (server.c).

---

## Task minigames

Eight task types are defined (`TaskType` in task.h) and assigned, shuffled, to
each innocent in `start_new_round()`:

```text
TASK_TIMER, TASK_CLICK, TASK_LETTER, TASK_REFLEX,
TASK_LOGICAL_ORDER, TASK_MEMORY, TASK_HOLD, TASK_ALTERNATE
```

An innocent starts a task by walking onto its tile and pressing `E`. The server
validates that the completed task matches the next expected task in the player's
shuffled order before crediting it. When every innocent completes all of their
tasks, the innocents win.

---

## Build

### Requirements

Install SDL2 and its modules: `SDL2`, `SDL2_image`, `SDL2_ttf`, `SDL2_mixer`,
`SDL2_net`. On Windows the Makefile targets an MSYS2/MinGW64 environment
(`/mingw64`); on macOS it uses Homebrew paths.

### Commands

**macOS / Linux:**
```bash
make all          # builds build/client and build/server
make clean        # remove the build/ directory
```

**Windows (MSYS2/MinGW):**
```bash
mingw32-make all
mingw32-make clean
```

Compiler flags are strict (`-Wall -Wextra -Wpedantic`). The binaries are written
to `build/`.

### Running

```bash
./build/server          # one host runs this
./build/client          # each player runs this; prompts for server IP
```

Use `127.0.0.1` for local testing, or the host's LAN IP for network play. The
working directory must contain the `assets/` folder (paths are relative).

---

## Controls

| Input       | Action                                            |
| ----------- | ------------------------------------------------- |
| `W A S D`   | Move                                              |
| `Space`     | Start game (host only, in lobby, 2+ players)      |
| `E`         | Interact: start a task on a task tile; open the meeting screen on the meeting table |
| `K`         | Kill nearest valid target (Killer, off cooldown)  |
| `R`         | Report a nearby body                              |
| `M`         | Toggle task map                                   |
| `TAB`       | Toggle task list panel                            |
| `I`         | Toggle controls overlay                           |
| `Q`         | Cancel an open task                               |
| `Escape`    | Pause menu / close menus                          |
| Mouse       | UI buttons (kill, report, meeting, vote)          |

(In `DEBUG` builds, `F1`/`F2` force an innocent/killer win.)

---
