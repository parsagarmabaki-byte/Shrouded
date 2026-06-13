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

## About the game

Shrouded plays out in short rounds for **3–6 players** on a single shared map.

**Roles.** When a round starts, the server secretly assigns exactly one
**Killer**; everyone else is an **Innocent**. Your role is revealed only to you
on a brief role screen, then everyone spawns into the map looking identical.

**The Innocents' job** is to walk around the map completing eight small task
minigames each, scattered across task tiles. They have no way to directly
identify the Killer — they have to deduce it from who they saw where, and from
bodies they find.

**The Killer's job** is to thin out the Innocents. The Killer can eliminate any
Innocent who is close and in front of them, then has to lie low through a kill
cooldown before striking again. Killing leaves a body behind on the map.

**Meetings.** Anyone can find a body and report it, and any living player can
call an emergency meeting from the meeting table. Either one freezes the map,
shows a short info screen, then opens a voting screen where every living player
votes for who they think the Killer is (or votes to skip). The player with the
most votes is eliminated; a tie or a skip-majority eliminates no one. Then
everyone respawns and the round continues.

**Winning.**
- **Innocents win** if they vote out / outlast the Killer, *or* if they
  collectively finish all of their tasks.
- **The Killer wins** the moment the living Killer count is greater than or
  equal to the living Innocent count — i.e. there are no longer enough
  Innocents to safely vote them out.

A typical round loops *running → (someone reports/calls a meeting) → vote →
running → …* until one side hits a win condition, after which the host can
start a fresh round without anyone reconnecting.

### The task minigames

Each Innocent is assigned all eight task types in a **shuffled order** and must
complete them in that order. Walk onto a task tile and press `E` to start it;
press `Q` to cancel. The eight types are:

| Task              | What you do                                                              |
| ----------------- | ------------------------------------------------------------------------ |
| **Timer**         | Wait out a short countdown — the one "task" that just needs patience.    |
| **Click**         | Click a target a set number of times.                                    |
| **Letter**        | Type a target string of letters in order; a wrong key restarts it.       |
| **Reflex**        | Stop a bouncing cursor inside a marked zone with `Space`; the zone shrinks each hit and you need several in a row. |
| **Logical order** | Click five numbers in ascending order; a mistake reshuffles them.        |
| **Memory**        | Watch a flashing arrow sequence, then repeat it with the arrow keys over three increasingly long rounds. |
| **Hold**          | Hold `Space` to fill a bar; let go early and it drains back down.        |
| **Alternate**     | Rapidly alternate between `A` and `D` — pressing the same key twice doesn't count. |

The **server validates every completion**: it checks that the task you finished
is the next one expected in *your* shuffled order before crediting it, so the
Killer can't fake task progress and an Innocent can't complete them out of
order. When all active Innocents have finished all eight tasks, the Innocents
win.

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

The exact win conditions (in `check_win_condition()`, server_game_logic.c) are:

- **Innocents win** if the killer is dead, **or** if the active innocents have
  collectively completed all their tasks (`completed_tasks >= active_innocents
  * TASK_COUNT`).
- **Killer wins** as soon as the living killer count is greater than or equal
  to the living innocent count (`alive_killer >= alive_innocents`).

The check runs after every kill, vote resolution, task completion, and player
leave, so a disconnect that tips the balance ends the round immediately.

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
| **UDP** | `MSG_PLAYER_SYNC_DATA`, `MSG_CLIENT_INPUT`, `MSG_GAME_STATE`, `MSG_MEETING_TIMER`, `MSG_JOIN` | every tick (~60/s) | low latency; a dropped packet is harmless because the next tick overwrites it |
| **TCP** | `MSG_TCP_HELLO`, `MSG_KILL_EVENT`, `MSG_EMERGENCY_MEETING`, `MSG_BODY_FOUND`, `MSG_PHASE_CHANGE`, `MSG_TASK_COMPLETE`, `MSG_VOTE_REQUEST`, `MSG_VOTE_UPDATE`, `MSG_MEETING_ENDED`, `MSG_KILL_READY`, `MSG_LEAVE` | on-event (rare) | must arrive exactly once and in order; losing one permanently diverges game state |

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

> All TCP messages the server sends are either a `PhaseChangeMsg` (used for
> every critical event type) or a `VoteUpdateMsg` (and the full `GameState` on
> lobby updates). The first 4 bytes are always the `MessageType`, which is how
> the receiver tells them apart.

#### TCP connection handshake (`MSG_TCP_HELLO`)

A client joins over **UDP** first (`MSG_JOIN`), and the server replies over UDP
with a `GameState` carrying that client's `local_player_id`. Only then does the
client know its id. Because the TCP connection is accepted *separately* (and in
an arbitrary order relative to the UDP join), the server cannot assume that
TCP socket slot `i` belongs to player `i`.

To bind the two, the client sends a `tcpHelloMessage` containing its
`player_id` over TCP as soon as it learns its id from the UDP `GameState`. The
server's `handle_tcp_vote_connections()` reads the hello and moves the socket
from its temporary accept slot into `tcpSockets[player_id]`, so every later TCP
message to that player is addressed by id. The same per-connection read loop
also handles `MSG_VOTE_REQUEST` (votes) and `MSG_LEAVE` (a clean disconnect);
a `recv` of `<= 0` on any connection is treated as a disconnect and routed
through `handle_leave_by_id()`.

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

A game requires **at least 3 active players** to start. This is enforced
server-side in `handle_start_game()` (server.c) — the client also greys out the
start prompt below 3, but the server is the authority and rejects an early
start request regardless of what the client sends.

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

You need two things: a **C compiler** and the **five SDL2 development
libraries** the project links against.

| Component        | Used for                                          |
| ---------------- | ------------------------------------------------- |
| `gcc` (or `clang` on macOS) | compiling the C source                 |
| `make`           | running the build                                 |
| `SDL2`           | window, input, rendering, timing                  |
| `SDL2_image`     | loading the PNG assets                            |
| `SDL2_ttf`       | text rendering (fonts in `assets/fonts`)          |
| `SDL2_mixer`     | sound effects and lobby music (`assets/SFX`)      |
| `SDL2_net`       | UDP + TCP networking                              |

> Install the **`-dev` / `-devel` / Homebrew** packages, not just the runtime
> libraries — you need the headers (`SDL2/SDL.h`, etc.) and the link archives,
> which only ship with the development packages. The Makefile expects the
> standard system include/lib locations for each platform (`/usr/include` on
> Linux, `/opt/homebrew` or `/usr/local` on macOS, `/mingw64` on Windows), so
> if you install SDL2 somewhere non-standard you'll need to add `-I`/`-L`
> paths to `CFLAGS`/`LDFLAGS`.

#### Linux

The compiler and Make usually come from your distro's build-essentials
package; then add the SDL2 dev libraries.

**Debian / Ubuntu:**
```bash
sudo apt update
sudo apt install build-essential        # gcc + make
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev \
                 libsdl2-mixer-dev libsdl2-net-dev
```

**Fedora:**
```bash
sudo dnf install gcc make
sudo dnf install SDL2-devel SDL2_image-devel SDL2_ttf-devel \
                 SDL2_mixer-devel SDL2_net-devel
```

**Arch / Manjaro:**
```bash
sudo pacman -S base-devel
sudo pacman -S sdl2 sdl2_image sdl2_ttf sdl2_mixer sdl2_net
```

#### macOS

Install the Xcode command line tools (they provide `clang`, which the Makefile
selects automatically on macOS, plus `make`), then the SDL2 libraries via
[Homebrew](https://brew.sh):

```bash
xcode-select --install                  # clang + make (skip if already installed)
brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer sdl2_net
```

Homebrew installs to `/opt/homebrew` on Apple Silicon and `/usr/local` on
Intel Macs; the Makefile already adds both as include/lib paths, so either
works without changes.

#### Windows (MSYS2 / MinGW64)

The Makefile targets a [MSYS2](https://www.msys2.org) MinGW64 environment, not
plain `cmd.exe`. After installing MSYS2, open the **"MSYS2 MinGW 64-bit"**
shell (important — not the default "MSYS" shell) and install the MinGW64
toolchain and the SDL2 packages:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
pacman -S mingw-w64-x86_64-SDL2 \
          mingw-w64-x86_64-SDL2_image \
          mingw-w64-x86_64-SDL2_ttf \
          mingw-w64-x86_64-SDL2_mixer \
          mingw-w64-x86_64-SDL2_net
```

Build and run from inside that same MinGW64 shell so the `/mingw64` paths in
the Makefile resolve correctly. The MinGW64 `make` is invoked as
`mingw32-make` (see the Commands section below).

#### Verifying your setup

Before building, confirm the compiler and SDL2 are visible:

```bash
gcc --version                 # or: clang --version  (macOS)
pkg-config --modversion sdl2  # prints the installed SDL2 version
```

If `pkg-config` reports an SDL2 version and `gcc`/`clang` runs, you're ready
to build. A missing header at compile time (e.g. `SDL2/SDL.h: No such file or
directory`) almost always means a `-dev` package wasn't installed; an
"undefined reference to `SDL_…`" error at the link step means a library is
installed but not being linked — recheck that all five `SDL2*` packages are
present.

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
to `build/`. A clean build currently produces a single warning — two
`-Wunused-parameter` notices on `debug_walls()` in `lib/src/wall_data.c`, a
debug-only helper. Everything else compiles warning-free.

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
| `Space`     | Start game (host only, in lobby, 3+ players)      |
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
