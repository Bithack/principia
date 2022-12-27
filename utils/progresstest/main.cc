#include "progress.hh"
#include <cstdlib>
#include <cstdio>
#include <ctime>

int main(void)
{
    srand(time(NULL));
    progress::init();
    printf("num in map 0: %d\n", progress::levels[0].size());
    printf("num in map 1: %d\n", progress::levels[1].size());
    printf("num in map 2: %d\n", progress::levels[2].size());

    lvl_progress *tmp;

    for (int x=0; x<1000; x++) {
        tmp = progress::get_level_progress(rand()%3, rand());

        tmp->completed = rand()%2;
        tmp->top_score = rand();
        tmp->last_score = rand();
        tmp->time = rand();
        tmp->num_plays = rand();
    }

    progress::commit();
}
