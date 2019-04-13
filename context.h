#ifndef context_h_INCLUDED
#define context_h_INCLUDED

#include "multiset.h"
#include "bitset.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define MAX_ROOMS 1024
#define MAX_PLAYERS 1024
#define ALPHABET_SIZE ('Z' - 'A' + 1)

#define SHM_NAME "/escape_room_mc394134"
#define SHM_SIZE sizeof(struct context)

struct game {
    struct bitset id_req;
    struct multiset type_req;
    struct multiset total_type_req;
    int n_players;
    int type;
    int room;
};

void game_clear(struct game *game);

struct player {
    sem_t mutex;
    sem_t sleep;
    int games_played;
    int current_game;
    int type;
    int message;
};

void player_init(struct player *player, int type);

struct room {
    sem_t mutex;
    sem_t sleep;
    int waiting;
    int inside;
    int size;
    int type;
    bool free;
};

void room_init(struct room *room, int size, int type);

struct context {
    int n_rooms;
    int n_players;

    sem_t mutex;
    sem_t sleep;

    int n_posting;

    sem_t exit_gate;
    int quit_message;

    int n_games;
    int game_list[MAX_PLAYERS];

    int n_waiting;
    int waiting_list[MAX_PLAYERS];

    struct bitset id_waiting;
    struct bitset id_needed;
    struct multiset type_waiting;
    struct multiset type_summary;
    struct multiset type_needed;

    int maxsize_waiting[ALPHABET_SIZE];
    int maxsize_summary[ALPHABET_SIZE];

    struct room rooms[MAX_ROOMS];
    struct player players[MAX_PLAYERS];
    struct game games[MAX_PLAYERS];
};

void context_init(struct context *ctx, int n_players, int n_rooms);

#endif //context_h_INCLUDED
