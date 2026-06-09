#ifndef NETWORK_DATA_H
#define NETWORK_DATA_H

#include <stdbool.h>
#include "task.h"

#define MAX_PLAYERS 6
#define VOTE_SKIP -1
#define SERVER_PORT 2000
#define SERVER_TCP_PORT (SERVER_PORT + 1)

#define SERVER_TICK_INTERVAL 0.016f

#define PLAYER_SPEED 200 // DELAD DATA MELLAN SERVER OCH CLIENT
#define PLAYER_SIZE 70

#define TASK_COUNT 8 // uppdatera  när fler tasks läggs till

// Milliseconds
#define MEETING_DURATION 120000
#define VOTE_RESULT_DURATION 10000
#define INFO_MEETING_DURATION 3000
#define SHOW_ROLE_DURATION 6000

// BARA RÖRELSER OCH ROLLER IMPLEMENTERADE
// MER SKA LÄGGAS TILL
typedef enum
{
    DIR_LEFT,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_UP
} Direction;

typedef enum
{
    MSG_JOIN,
    MSG_LEAVE,
    MSG_START_GAME,
    MSG_PLAY_AGAIN,
    MSG_CLIENT_INPUT,
    MSG_GAME_STATE,
    MSG_READY_STATUS,
    MSG_KILL_REQUEST,
    MSG_KILL_EVENT,
    MSG_EMERGENCY_MEETING,
    MSG_BODY_FOUND,
    MSG_TASK_COMPLETE,
    MSG_VOTE_REQUEST,
    MSG_DEBUG_CREWMATES_WIN,
    MSG_DEBUG_IMPOSTOR_WIN,
    MSG_PLAYER_SYNC_DATA,
    MSG_KILL_READY,
    MSG_PHASE_CHANGE,
    MSG_VOTE_UPDATE,
    MSG_MEETING_ENDED,
    MSG_MEETING_TIMER
} MessageType;

typedef enum
{
    MEETING_NONE,
    MEETING_EMERGENCY,
    MEETING_BODY
} MeetingReason;

typedef struct
{
    int x;
    int y;
} Position;

typedef struct
{
    int active;
    int player_id;

    float x;
    float y;

    int isAlive;
    int isKiller;
    int isDoingTask;
    
    int emergency_meeting;

    int current_frame;
    Direction direction;

    TaskType task_order[TASK_COUNT]; // shuffled player task list
    int tasks_completed;
} playerState;

typedef enum
{
    GAME_LOBBY,
    GAME_RUNNING,
    GAME_SHOW_ROLE,
    GAME_INFO_MEETING,
    GAME_MEETING,
    SHOW_VOTE_RESULT,
    GAME_CREWMATES_WIN,
    GAME_KILLER_WIN
} gamePhase;

typedef struct
{
    MessageType type;
    playerState players[MAX_PLAYERS];
    gamePhase phase;
    int local_player_id;
    int host_player_id;
    MeetingReason meeting_reason;
    int emergency_meeting_reported_id;
    int voting_results[MAX_PLAYERS + 1];
    int voting_result;
    int total_tasks_completed;
    int meeting_time_remaining; // milliseconds left in meeting
    bool kill_cooldown_active;

} gameState;

typedef struct
{
    MessageType type;
    gamePhase phase;
    int killer_id;
    int victim_id;
    float x;
    float y;
} KillEventMsg;

typedef struct
{
    MessageType type;
    int target_id;
    int voter_id;
} VoteRequest;

typedef struct
{
    MessageType type;
    gamePhase phase;
    MeetingReason meeting_reason;
    int emergency_meeting_reported_id;
    int voting_results[MAX_PLAYERS + 1];
    int voting_result;
    int player_voted[MAX_PLAYERS];
    int player_alive[MAX_PLAYERS];
} VoteUpdateMsg;

typedef struct
{
    int alive_players_id[MAX_PLAYERS];
    int alive_players_count;

    int has_voted[MAX_PLAYERS];

    int votes_recieved;
    VoteRequest votes[MAX_PLAYERS];

} Meeting;

typedef struct
{
    MessageType type;
    int meeting_time_remaining;
} MeetingTimer;


typedef struct
{
    MessageType type;
} EmergencyMeetingMsg;

typedef struct
{
    MessageType type;
    int player_id;
}TaskCompletedEvent;


typedef struct
{
    MessageType type;
    gamePhase phase;
    MeetingReason meeting_reason;
    int emergency_meeting_reported_id;
} EmergencyMeetingEvent;

typedef struct
{
    MessageType type;
    gamePhase phase;
    MeetingReason meeting_reason;
    int voting_results[MAX_PLAYERS + 1];
    int voting_result;
} MeetingEndedEvent;


typedef struct
{
    MessageType type;

} KillReadyMsg;

typedef struct
{
    MessageType type;
    gamePhase phase;
    MeetingReason meeting_reason;
    int player_id;
    int victim_id;
} PhaseChangeMsg;

typedef struct
{
    MessageType type;

    int player_id;

    float x;
    float y;

    int current_frame;

    Direction direction;

} PlayerUpdateMsg;

typedef struct 
{
    MessageType type;
    PlayerUpdateMsg player[MAX_PLAYERS];
}PlayerSyncMsg;


typedef struct
{
    MessageType type;
    int player_id;

    int up;
    int down;
    int left;
    int right;

    int current_frame;
    Direction direction;
} InputMsg;

typedef struct
{
    MessageType type;
    int body_id;
} ReportBodyMsg;

typedef struct
{
    MessageType type;
    int target_id;
} KillRequestMsg;

typedef struct
{
    MessageType type;
} joinMessage;
typedef struct
{
    MessageType type;
} leaveMessage;
typedef struct
{
    MessageType type;
} startGameMessage;
typedef struct
{
    MessageType type;
    int player_id;
    TaskType task_type;
} TaskCompleteMsg;
#endif
