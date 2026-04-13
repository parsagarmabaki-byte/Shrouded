#ifndef NETWORK_DATA_H
#define NETWORK_DATA_H

#define MAX_PLAYERS 6
#define SERVER_PORT 2000

#define PLAYER_SPEED 200 //DELAD DATA MELLAN SERVER OCH CLIENT
#define PLAYER_SIZE 70

// BARA RÖRELSER OCH ROLLER IMPLEMENTERADE
// MER SKA LÄGGAS TILL

typedef enum{
    MSG_JOIN,
    MSG_LEAVE,
    MSG_START_GAME,
    MSG_CLIENT_INPUT,
    MSG_GAME_STATE,
    MSG_READY_STATUS,
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
} clientInput; 

typedef struct {
    int active;
    int player_id;
    float x;
    float y;
    int isAlive;
    int isImpostor;
    int isDoingTask;
} playerState;

typedef enum{
    GAME_LOBBY,
    GAME_RUNNING,
    GAME_MEETING
} gamePhase;

typedef struct {
    MessageType type;
    playerState players[MAX_PLAYERS];
    gamePhase phase;
    int local_player_id;
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