#include <iostream>
#include "progress.hh"

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("usage: progress-get [path_to_data_bin] [community-level-id]\n");
        return 1;
    }

    progress::init(argv[1]);

    const lvl_progress *p = progress::get_level_progress(1, atoi(argv[2]));

    // completed, num_plays, last_score
    std::cout << (int)p->completed << ',' << p->num_plays << ',' << p->last_score;

    return 0;
}
