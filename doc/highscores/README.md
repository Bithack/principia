All highscore data is stored in the impregnable data.bin.

When we submit highscores to the server, we submit the data.bin file directly.

However, it's possible to cheat by modifying the memory where the score number is located before submitting your score.

**SCORE SUBMISSIONS NEED TO BE DISABLED FOR STATE LOADS!!!!!!!**

To circumvent this, we will store extra verification numbers that are modified at the same time as ```G->state.score```.

``G->state.score`` should therefore never be updated directly, but should use the helper function we will create in game,
``G->set_score(int new_score)``, ``int G->get_score()``, ``bool G->verify_score()``, ``G->add_score(int score_mod)``.

In addition to this, we should insert some data into data.bin before we submit it.

What we need is:
 * crc32 of the level buf (so the level hasn't been modified)
 * revision of level
 * local timestamp

So, how do we store this data?
We push it into some fake position of data.bin.
```c++
int base_fake_id = 1402
forward = 1
backwards = -1
int dir = forward
switch (community_id % 5) {
  case 0:
    fake_level_id = base_fake_id + 4
    dir = forward
    
    case 1:
    fake_level_id = base_fake_id
    dir = forward
    
    case 2:
    fake_level_id = base_fake_id + 1
    dir = backwards
    
    case 3:
    fake_level_id = base_fake_id + 1
    dir = forward
    
    case 4:
    fake_level_id = base_fake_id + 6
    dir = backwards
}

char crcdata[32] = crc32(level.buf + level.name + level.revision + local_timestamp);
for (int x=0; x<32; ++x) {
  progress[fake_level_id+(x*dir)].top_score = crcdata[x];
}

progress[fake_level_id-3].last_score = level.revision
progress[fake_level_id+4].last_score = local_timestamp
```
