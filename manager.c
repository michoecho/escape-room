#include "context.h"
#include "ioutils.h"
#include <sys/wait.h>

struct context *ctx;

void launch_players() {
    for (int i = 1; i <= ctx->n_players; ++i) {
        pid_t pid = fork();
        check(pid);
        if (pid == 0) {
            char buf[20];
            sprintf(buf, "%d", i);
            check(execl("./player", "./player", buf, (char*)NULL));
        }
    }
}

void handle_logout() {
    for (int i = 0; i < ctx->n_players; ++i) {
        wait(0);
        int id = ctx->quit_message;
        printf("Player %d left after %d game(s)\n", id + 1, ctx->players[id].games_played);
        sem_post(&ctx->exit_gate);
    }
}

void prepare_context() {
    int n_players;
    int n_rooms;
    scanf("%d %d", &n_players, &n_rooms);
    context_init(ctx, n_players, n_rooms);
}

void prepare_rooms() {
    for (int i = 0; i < ctx->n_rooms; ++i) {
        char type_char;
        int size;
        scanf(" %c %d", &type_char, &size);
        int type = type_char - 'A';

        room_init(&ctx->rooms[i], size, type);

        if (ctx->maxsize_summary[type] < size) {
            ctx->maxsize_summary[type] = size;
            ctx->maxsize_waiting[type] = size;
        }
    }
}

void create_shm() {
    int flags = MAP_SHARED;
    int prot = PROT_READ | PROT_WRITE;

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    check(shm_fd);
    check(ftruncate(shm_fd, SHM_SIZE));

    ctx = mmap(NULL, SHM_SIZE, prot, flags, shm_fd, 0);
    check((ctx == MAP_FAILED) ? -1 : 0);

    check(close(shm_fd));
}

void destroy_shm() {
    check(munmap(ctx, SHM_SIZE));
    check(shm_unlink(SHM_NAME));
}

int main() {
    create_shm();
    prepare_context();
    prepare_rooms();
    launch_players();
    handle_logout();
    destroy_shm();
}
