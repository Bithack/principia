#include "progress.hh"
#include <cstdlib>
#include <cstdio>
#include <ctime>

int main(int argc, char **argv)
{
    if (argc < 1) {
        return 1;
    }

    if (argc < 2) {
        printf("usage: %s [community-level-id]\n", argv[0]);
        return 1;
    }

    progress::init();
    //fprintf(stderr, "opening DB level %u", atoi(argv[1]));
    lvl_progress *p = progress::get_level_progress(1, atoi(argv[1]));

    printf("completed: %s, top score: %u, num plays %u\n", p->completed?"YES":"NO", p->top_score, p->num_plays);

    return 0;
}
