#ifndef NETWORK_DATA_H
#define NETWORK_DATA_H

#define MAX_PLAYERS 6
#define SERVER_PORT 2000

#define SERVER_TICK_INTERVAL 0.016f

#define PLAYER_SPEED 200 //DELAD DATA MELLAN SERVER OCH CLIENT
#define PLAYER_SIZE 85

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
} MessageType;

typedef enum {
    DIR_LEFT,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_UP
} Direction;

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
<<<<<<< HEAD
    Direction direction;
=======
>>>>>>> 9f7b99b535e4cd9152ce8fcf2961e37b03032f3b
} clientInput; 



typedef struct {
    int active;
    int player_id;
    float x;
    float y;
    int isAlive;
    int isImpostor;
    int isDoingTask;
    int current_frame;
<<<<<<< HEAD
=======
    float animation_timer;
>>>>>>> 9f7b99b535e4cd9152ce8fcf2961e37b03032f3b
    Direction direction;
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