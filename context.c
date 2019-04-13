#include "context.h"
#include <string.h>

void game_clear(struct game *game) {
    bitset_clear(&game->id_req);
    multiset_clear(&game->type_req);
    multiset_clear(&game->total_type_req);
    game->n_players = 0;
    game->type = -1;
    game->room = -1;
}

void room_init(struct room *room, int size, int type) {
    sem_init(&room->mutex, 1, 1);
    sem_init(&room->sleep, 1, 0);
    room->waiting = 0;
    room->inside = 0;
    room->size = size;
    room->type = type;
    room->free = true;
}

void player_init(struct player *player, int type) {
    sem_init(&player->mutex, 1, 1);
    sem_init(&player->sleep, 1, 0);
    player->games_played = 0;
    player->current_game = -1;
    player->type = type;
    player->message = 0;
}

void context_init(struct context *ctx, int n_players, int n_rooms) {
    memset(ctx, 0, sizeof(*ctx));

    ctx->n_rooms = n_rooms;
    ctx->n_players = n_players;

    sem_init(&ctx->mutex, 1, 1);
    sem_init(&ctx->sleep, 1, 0);

    ctx->n_posting = 0;

    sem_init(&ctx->exit_gate, 1, 1);
    ctx->quit_message = -1;

    ctx->n_games = 0;

    bitset_clear(&ctx->id_waiting);
    bitset_clear(&ctx->id_needed);
    multiset_clear(&ctx->type_waiting);
    multiset_clear(&ctx->type_summary);
    multiset_clear(&ctx->type_needed);
}
