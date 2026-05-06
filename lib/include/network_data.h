#ifndef NETWORK_DATA_H
#define NETWORK_DATA_H

#include <stdbool.h>
#include "task.h"

#define MAX_PLAYERS 6
#define VOTE_SKIP -1
#define SERVER_PORT 2000

#define SERVER_TICK_INTERVAL 0.016f

#define PLAYER_SPEED 200 //DELAD DATA MELLAN SERVER OCH CLIENT
#define PLAYER_SIZE 70

#define TASK_COUNT 6 // uppdatera  när fler tasks läggs till


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
    MSG_EMERGENCY_MEETING,
    MSG_BODY_FOUND,
    MSG_TASK_COMPLETE,
    MSG_VOTE_REQUEST
} MessageType;


typedef struct 
{
    int x;
    int y;
} Position;


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
    Position dead_body;
    int target_id;
    
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

    TaskType task_order[TASK_COUNT];   // shuffled player task list
    int tasks_completed;
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

    int total_tasks_completed;
} gameState;

typedef struct
{
    MessageType type;
    int target_id;
    int voter_id;
} VoteRequest;

typedef struct
{
    int alive_players_id[MAX_PLAYERS];
    int alive_players_count;

    int has_voted[MAX_PLAYERS];

    int votes_recieved;
    VoteRequest votes[MAX_PLAYERS];

} Meeting;

typedef struct {
    MessageType type;
} joinMessage;
typedef struct{
    MessageType type;
} leaveMessage;
typedef struct{
    MessageType type;
} startGameMessage;
typedef struct {
    MessageType type;
    int player_id;
    TaskType task_type;
} TaskCompleteMsg;
#endif