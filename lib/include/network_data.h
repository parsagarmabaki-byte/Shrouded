#ifndef NETWORK_DATA_H
#define NETWORK_DATA_H

#include <stdbool.h>

#define MAX_PLAYERS 6
#define SERVER_PORT 2000

#define SERVER_TICK_INTERVAL 0.016f

#define PLAYER_SPEED 200 //DELAD DATA MELLAN SERVER OCH CLIENT
#define PLAYER_SIZE 75
#define PLAYER_HITBOX_SIZE 30


// BARA RÖRELSER OCH ROLLER IMPLEMENTERADE
// MER SKA LÄGGAS TILL
typedef enum {
    DIR_LEFT,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_UP
} Direction;

typedef enum{
    MSG_JOIN,
    MSG_LEAVE,
    MSG_START_GAME,
    MSG_CLIENT_INPUT,
    MSG_GAME_STATE,
    MSG_READY_STATUS,
    MSG_KILL_REQUEST,
    MSG_KILL_EVENT,
    MSG_EMERGENCY_MEETING
} MessageType;


typedef struct { // Info som användaren klickar in
    MessageType type;
    int player_id;
    int up;
    int down;
    int left;
    int right;
    int interact;
    int kill;
    int report;
    int current_frame;
    int isAlive;
    int emergency_meeting_left;
    Direction direction;
} clientInput; 


typedef struct
{
    MessageType type;
    int killer_id;
    int victim_id;
    float x;
    float y;
} KillEventMsg;

typedef struct {
    int active;
    int player_id;
    
    float x;
    float y;

    int isAlive;
    int isImpostor;
    int isDoingTask;

    bool kill_cooldown_active;
    Uint32 kill_cooldown_start;

    int emergency_meeting;

    int current_frame;
    Direction direction;
} playerState;

typedef enum{
    GAME_LOBBY,
    GAME_RUNNING,
    GAME_SHOW_ROLE,
    GAME_INFO_MEETING,
    GAME_MEETING,
    GAME_CREWMATES_WIN,
    GAME_IMPOSTOR_WIN
} gamePhase;

typedef struct {
    MessageType type;
    playerState players[MAX_PLAYERS];
    gamePhase phase;
    int local_player_id;
    int emergency_meeting_reported_id;
} gameState;

typedef struct {
    MessageType type;
} joinMessage;
typedef struct{
    MessageType type;
} leaveMessage;
typedef struct{
    MessageType type;
} startGameMessage;
#endif