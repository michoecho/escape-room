#include "context.h"
#include "ioutils.h"
#include <ctype.h>
#include <assert.h>
#include <string.h>

struct context *ctx;
int my_id;
int my_type;
struct player *my_self;
struct game *my_game;
FILE *input;
FILE *output;
bool posting = true;

#define MESSAGE_QUIT 4
#define MESSAGE_POST 2
#define MESSAGE_PLAY 1

void read_my_id(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "1 argument expected");
        exit(EXIT_FAILURE);
    }
    my_id = strtol(argv[1], NULL, 10);
    if (my_id < 1 || MAX_PLAYERS < my_id) {
        fprintf(stderr, "Invalid id");
        exit(EXIT_FAILURE);
    }
    my_id -= 1;
}

void open_shm() {
    int flags = MAP_SHARED;
    int prot = PROT_READ | PROT_WRITE;

    int shm_fd = shm_open(SHM_NAME, O_RDWR, S_IRUSR | S_IWUSR);
    check(shm_fd);  

    ctx = mmap(NULL, SHM_SIZE, prot, flags, shm_fd, 0);
    check((ctx == MAP_FAILED) ? -1 : 0);

    check(close(shm_fd));
}

void open_files() {
    char filename[64];
    sprintf(filename, "player-%d.in", my_id + 1);
    input = fopen(filename, "r");
    sprintf(filename, "player-%d.out", my_id + 1);
    output = fopen(filename, "w");
}

void read_my_type() {
    int c = skip_space_fgetc(input);
    my_type = c - 'A';
    skip_line(input);
}

void init_self() {
    my_self = &ctx->players[my_id];
    my_game = &ctx->games[my_id];
    player_init(my_self, my_type);
    my_self->message = MESSAGE_POST;
}

void close_shm() {
    check(munmap(ctx, SHM_SIZE));
}

void close_files() {
    fclose(input);
    fclose(output);
}

void mark_self_as_waiting() {
    bitset_add(&ctx->id_waiting, my_id);
    multiset_add(&ctx->type_waiting, my_type);
    ctx->waiting_list[ctx->n_waiting] = my_id;
    ++ctx->n_waiting;
}

void login() {
    sem_wait(&ctx->mutex);
    multiset_add(&ctx->type_summary, my_type);
    mark_self_as_waiting();
    ++ctx->n_posting;
    if (ctx->n_posting < ctx->n_players) {
        sem_post(&ctx->mutex);
        sem_wait(&ctx->sleep);
    } else {
        sem_post(&ctx->mutex); // Cascade
    }
    sem_post(&ctx->sleep);
}

void logout() {
    sem_wait(&ctx->exit_gate);
    ctx->quit_message = my_id;
}

bool try_fetch_game() {
    game_clear(my_game);
    
    int c = skip_space_fgetc(input);
    my_game->type = c - 'A';

    bitset_add(&my_game->id_req, my_id);
    multiset_add(&my_game->total_type_req, my_type);
    my_game->n_players += 1;

    while ((c = skip_space_fgetc(input)) != '\n') { 
        if (c >= 'A' && c <= 'Z') {
            int type = c - 'A';
            if (multiset_get(&my_game->total_type_req, type) >= ctx->n_players) {
                skip_line(input);
                return false;
            }
            multiset_add(&my_game->type_req, type);
            multiset_add(&my_game->total_type_req, type);
        } else if (c >= '0' && c <= '9') {
            int id;
            ungetc(c, input);
            fscanf(input, "%d", &id);
            id -= 1;
            if (id >= ctx->n_players) {
                skip_line(input);
                return false;
            }
            bitset_add(&my_game->id_req, id);
            multiset_add(&my_game->total_type_req, ctx->players[id].type);
        }
        my_game->n_players += 1;
    }

    if (ctx->maxsize_summary[my_game->type] < my_game->n_players) {
        return false;
    }

    if (!multiset_subset(&ctx->type_summary, &my_game->total_type_req)) {
        return false;
    }

    return true;
}

bool fetch_game() {
    while (!is_eof(input)) {
        fpos_t line_beg;
        fgetpos(input, &line_beg);

        if (try_fetch_game()) {
            return true;
        } else {
            fsetpos(input, &line_beg);
            fprintf(output, "Invalid game \"");
            copy_line(input, output);
            fprintf(output, "\"\n");
        }
    }

    return false;
}

bool can_be_started(struct game *game) {
    return multiset_subset(&ctx->type_waiting, &game->total_type_req)
        && bitset_subset(&ctx->id_waiting, &game->id_req)
        && ctx->maxsize_waiting[game->type] >= game->n_players;
}

void claim_players(struct game *game) {
    multiset_subtract(&ctx->type_waiting, &game->total_type_req);
    bitset_subtract(&ctx->id_waiting, &game->id_req);

    int back = 0;
    for (int front = 0; front < ctx->n_waiting; ++front) {
        int id = ctx->waiting_list[front];
        int type = ctx->players[id].type;
        if (multiset_get(&game->type_req, type) > 0 && bitset_test(&ctx->id_waiting, id)) {
            bitset_add(&game->id_req, id);
            bitset_rm(&ctx->id_waiting, id);
            multiset_rm(&game->type_req, type);
        } else if (!bitset_test(&game->id_req, id)) {
            ctx->waiting_list[back] = ctx->waiting_list[front];
            ++back;
        } 
    }
    ctx->n_waiting -= game->n_players;
}

void find_maxsize_waiting(int type) {
    int newmax_size = 0;
    for (int i = 0; i < ctx->n_rooms; ++i) {
        struct room *room = &ctx->rooms[i];
        if (room->free &&
            room->type == type &&
            room->size > newmax_size) {
            newmax_size = room->size;
        }
    }
    ctx->maxsize_waiting[type] = newmax_size;
}

void claim_room(struct game *game) {
    int choice = -1;
    int choice_size = MAX_PLAYERS;
    for (int i = 0; i < ctx->n_rooms; ++i) {
        struct room *room = &ctx->rooms[i];
        if (room->free &&
            room->type == game->type &&
            room->size >= game->n_players &&
            room->size < choice_size) {

            choice = i;
            choice_size = room->size;
        }
    }

    ctx->rooms[choice].free = false;

    if (choice_size == ctx->maxsize_waiting[game->type]) {
        find_maxsize_waiting(game->type);
    }

    game->room = choice;
}

void register_game(int game_id) {
    ctx->game_list[ctx->n_games] = game_id;
    ++ctx->n_games;
}

void unregister_game(int list_index) {
    --ctx->n_games;
    for (int i = list_index; i < ctx->n_games; ++i) {
        ctx->game_list[i] = ctx->game_list[i + 1];
    }
}

int find_startable_game() {
    for (int i = 0; i < ctx->n_games; ++i) {
        int game_id = ctx->game_list[i];
        if (can_be_started(&ctx->games[game_id])) {
            unregister_game(i);
            return game_id;
        }
    }
    return -1;
}

void wake_players(int game_id) {
    struct game *game = &ctx->games[game_id];
    for (int i = 0; i < ctx->n_players; ++i) {
        if (bitset_test(&game->id_req, i)) {
            struct player *player = &ctx->players[i];
            sem_wait(&player->mutex);
            player->message |= MESSAGE_PLAY;
            player->current_game = game_id;
            sem_post(&player->mutex);
            sem_post(&player->sleep);
        }
    }
}

void announce_game(struct bitset *players, int owner, int room) {
    fprintf(output, "Game defined by %d is going to start: room %d, players ", owner + 1, room + 1);
    bitset_print(players, 1, output);
    fputc('\n', output);
}

void recalculate_needed() {
    bitset_clear(&ctx->id_needed);
    multiset_clear(&ctx->type_needed);
    for (int i = 0; i < ctx->n_games; ++i) {
        int game_id = ctx->game_list[i];
        struct game *game = &ctx->games[game_id];
        multiset_sum(&ctx->type_needed, &game->type_req);
        bitset_sum(&ctx->id_needed, &game->id_req);
    }
}

bool player_needed(int id) {
    int type = ctx->players[id].type;
    return multiset_get(&ctx->type_needed, type) || bitset_test(&ctx->id_needed, id);
}

void release_unneeded() {
    if (ctx->n_posting > 0)
        return;
    recalculate_needed();
    for (int i = 0; i < ctx->n_players; ++i) {
        if (!player_needed(i)) {
            struct player *player = &ctx->players[i];
            sem_wait(&player->mutex);
            player->message |= MESSAGE_QUIT;
            sem_post(&player->mutex);
            sem_post(&player->sleep);
        }
    }
}

void try_start_game() {
    sem_wait(&ctx->mutex);
    int game_id = find_startable_game();
    if (game_id < 0) {
        sem_post(&ctx->mutex);
        return;
    }
    struct game *game = &ctx->games[game_id];

    claim_players(game);
    release_unneeded();
    claim_room(game);
    struct bitset woken_players = game->id_req;
    int room = game->room;

    wake_players(game_id);
    sem_post(&ctx->mutex);
    announce_game(&woken_players, game_id, room);
}

void announce_entrance(struct bitset *players, int owner, int room) {
    fprintf(output, "Entered room %d, game defined by %d, waiting for players ", room + 1, owner + 1);
    bitset_print(players, 1, output);
    fputc('\n', output);
}

void announce_leave(int room) {
    fprintf(output, "Left room %d\n", room + 1);
}

void play() {
    ++my_self->games_played;
    struct game *game = &ctx->games[my_self->current_game];
    int room_id = game->room;
    struct room *room = &ctx->rooms[room_id];
    sem_wait(&room->mutex);
    bitset_rm(&game->id_req, my_id);
    struct bitset remaining_players = game->id_req;
    sem_post(&room->mutex);
    announce_entrance(&remaining_players, my_self->current_game, room_id);

    sem_wait(&room->mutex);
    if (++room->waiting < game->n_players) {
        sem_post(&room->mutex);
        sem_wait(&room->sleep);
    }
    ++room->inside;
    if (--room->waiting > 0) {
        sem_post(&room->sleep);
    } else {
        sem_post(&room->mutex);
    }

    // Playing

    sem_wait(&room->mutex);
    if (--room->inside == 0) {
        sem_wait(&ctx->mutex);
        room->free = true;
        if (ctx->maxsize_waiting[room->type] < room->size)
            ctx->maxsize_waiting[room->type] = room->size;
        sem_post(&ctx->mutex);
        try_start_game();

        struct player *owner = &ctx->players[my_self->current_game];
        sem_wait(&owner->mutex);
        owner->message |= MESSAGE_POST;
        sem_post(&owner->mutex);
        sem_post(&owner->sleep);
    }
    sem_post(&room->mutex);
    announce_leave(room_id);
}

void participate() {
    while (true) {
        sem_wait(&my_self->mutex);
        int message = my_self->message;
        my_self->message = 0;
        sem_post(&my_self->mutex);

        if (message & MESSAGE_POST) {
            if (posting) {
                if (fetch_game()) {
                    sem_wait(&ctx->mutex);
                    register_game(my_id);
                    sem_post(&ctx->mutex);
                    try_start_game();
                }
                if (feof(input)) {
                    posting = false;
                    sem_wait(&ctx->mutex);
                    --ctx->n_posting;
                    release_unneeded();
                    sem_post(&ctx->mutex);
                }
            }
        }
        if (message & MESSAGE_PLAY) {
            play();
            sem_wait(&ctx->mutex);
            mark_self_as_waiting();
            sem_post(&ctx->mutex);
            try_start_game();
        }
        if (message & MESSAGE_QUIT) {
            return;
        }

        sem_wait(&my_self->sleep);
    }
}

int main(int argc, char *argv[]) {
    read_my_id(argc, argv);
    open_shm();
    open_files();
    read_my_type();
    init_self();
    login();
    participate();
    logout();
    close_shm();
    close_files();
}
