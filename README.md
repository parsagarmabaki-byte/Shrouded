# Shrouded

Shrouded is a LAN-based multiplayer social deduction game inspired by the gameplay structure of *Among Us*. The project was developed in C using SDL2 as part of the CM1008 project course at KTH.

The game uses a client-server architecture over UDP where one player becomes the **Killer** while the remaining players act as **Crewmates**. Crewmates must complete tasks, report bodies, participate in emergency meetings, and identify the Killer before the crew is eliminated.

---

# Features

## Multiplayer Networking

* UDP-based client-server architecture using SDL_net
* Server-authoritative game state synchronization
* LAN multiplayer support
* Player join and disconnect handling
* Real-time player movement synchronization
* Voting and meeting synchronization
* Packet-loss handling for critical interactions such as voting

## Gameplay Systems

### Crewmate Features

* Complete multiple task minigames
* Report dead bodies
* Call emergency meetings
* Vote during meetings
* Ghost state after death

### Killer Features

* Eliminate nearby players
* Use kill cooldown mechanics
* Blend in during meetings and gameplay

### Meeting System

* Emergency meeting screen
* Body report flow
* Voting system
* Skip vote support
* Tie vote handling
* Voting result screen
* Player elimination system

---

# Implemented Task Minigames

The game currently contains multiple local minigames:

1. Timer task
2. Click task
3. Typing task
4. Reflex task
5. Logical order task
6. Memory task

Task completion contributes to the crewmates' win condition.

---

# Technical Features

* Camera system with player follow
* Sprite animations
* Collision detection and wall systems
* Custom UI rendering
* Audio playback using SDL_mixer
* Lobby system and role reveal screens
* Win screens for both teams
* Task map overlay
* Transparent UI button rendering
* Cross-platform support

---

# Repository Structure

```text
.
|-- Makefile
|-- README.md
|-- assets/
|-- Fonts/
|-- build/
|-- client/
|   `-- src/
|-- server/
|   |-- include/
|   `-- src/
|-- lib/
|   |-- include/
|   `-- src/
```

## Directory Overview

### `client/src/`

Contains the client-side networking, rendering, input handling, lobby flow, and local gameplay logic.

### `server/src/`

Contains the authoritative server implementation including:

* game loop
* packet handling
* voting logic
* meeting management
* round transitions
* win-condition validation

### `lib/include/`

Shared headers and public interfaces used by both client and server.

### `lib/src/`

Shared gameplay systems including:

* rendering
* player movement
* task systems
* emergency meetings
* map logic
* audio systems
* networking helpers

### `assets/`

Contains all game assets including:

* sprites
* UI assets
* sounds
* voting result assets
* lobby backgrounds
* win screens

---

# Architecture

Shrouded uses a server-authoritative architecture.

The server controls:

* player states
* positions
* role assignment
* alive/dead status
* task progress
* meeting states
* voting results
* win conditions

Clients send user input and gameplay requests while rendering the latest synchronized game state locally.

This prevents clients from directly controlling important gameplay state.

---

# Game State Flow

The game uses a shared `gamePhase` enum to transition between gameplay states.

```c
GAME_LOBBY
GAME_SHOW_ROLE
GAME_RUNNING
GAME_INFO_MEETING
GAME_MEETING
SHOW_VOTE_RESULT
GAME_CREWMATES_WIN
GAME_KILLER_WIN
```

## Typical Game Flow

```text
GAME_LOBBY
    ↓
GAME_SHOW_ROLE
    ↓
GAME_RUNNING
    ↓
GAME_INFO_MEETING
    ↓
GAME_MEETING
    ↓
SHOW_VOTE_RESULT
    ↓
GAME_RUNNING
```

The game eventually transitions into either:

* `GAME_CREWMATES_WIN`
* `GAME_KILLER_WIN`

---

# Networking

The project uses SDL_net with UDP sockets.

Shared packet definitions are located in:

```text
lib/include/network_data.h
```

## Important Packet Types

```text
MSG_JOIN
MSG_LEAVE
MSG_START_GAME
MSG_CLIENT_INPUT
MSG_GAME_STATE
MSG_KILL_REQUEST
MSG_KILL_EVENT
MSG_EMERGENCY_MEETING
MSG_BODY_FOUND
MSG_TASK_COMPLETE
MSG_VOTE_REQUEST
```

The server continuously broadcasts synchronized game state updates to connected clients.

---

# Build Instructions

## Requirements

Install the following libraries:

* SDL2
* SDL2_image
* SDL2_ttf
* SDL2_mixer
* SDL2_net

---

## Build the Project

```bash
make all
```

Generated executables:

```text
build/client
build/server
```

---

## Additional Make Targets

```bash
make run_server
make run_client
make clean
```

---

# Running the Game

## Start the Server

```bash
./build/server
```

## Start the Client

```bash
./build/client
```

The client prompts for a server IP address.

For local testing:

```text
127.0.0.1
```

For LAN play, enter the host computer's local IP address.

---

# Controls

| Input       | Action                             |
| ----------- | ---------------------------------- |
| `W A S D`   | Move                               |
| `Space`     | Start game in lobby                |
| `Escape`    | Exit or close menus                |
| `M`         | Toggle task map                    |
| `E`         | Open emergency meeting and task interaction |
| `K`         | Kill nearby player as Killer       |
| `R`         | Report nearby body                 |
| `Q`         | Cancel active task                 |
| `1`         | Start timer task                   |
| `2`         | Start click task                   |
| `3`         | Start typing task                  |
| `4`         | Start reflex task                  |
| `5`         | Start logical order task           |
| `6`         | Start memory task                  |
| Mouse click | Interact with UI buttons and tasks |

---

# Tech Stack

| Area                 | Technology            |
| -------------------- | --------------------- |
| Programming Language | C                     |
| Graphics             | SDL2, SDL2_image      |
| Text Rendering       | SDL2_ttf              |
| Audio                | SDL2_mixer            |
| Networking           | SDL_net               |
| Build System         | Make                  |
| Platforms            | Windows, Linux, macOS |

---
