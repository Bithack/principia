#include "progress.hh"
#include <cstdlib>
#include <cstdio>
#include <ctime>

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: progress-dump <path-to-data-bin>\n");
        return 1;
    }

    srand(time(NULL));
    progress::init(argv[1]);

    for (int x = 0; x < 3; x++) {
        printf("Level type %d (%lu levels):\n", x, progress::levels[x].size());
        for (std::map<uint32_t, lvl_progress*>::iterator i = progress::levels[x].begin();
                i != progress::levels[x].end(); i++) {
            uint32_t id = i->first;
            lvl_progress *p = i->second;

            printf("  Level ID: %u\n", id);
            printf("    Completed: %s\n", p->completed ? "Yes" : "No");
            printf("    Num plays: %u\n", p->num_plays);
            printf("    Top score: %u\n", p->top_score);
            printf("    Last score: %u\n", p->last_score);
            printf("    Time: %u seconds\n", p->time);
        }
    }
}
