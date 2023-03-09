#include <stdio.h>
#include "../src/src/pkgman.hh"

#define MAX_READ (4096*18)

static lvledit lvl;
static char input[MAX_READ];
static char path[1024];
static size_t input_len = 0;

static void _get_input(void)
{
    int c;
    while ((c = getchar()) != EOF) {
        input[input_len] = (char)c;
        input_len++;
    }
}

static void get_description(void)
{
    printf("%.*s", lvl.lvl.descr_len, lvl.lvl.descr);
}

static void set_description(void)
{
    _get_input();
    lvl.lvl.descr_len = input_len;
    if (lvl.lvl.descr) free(lvl.lvl.descr);
    lvl.lvl.descr = input;
    lvl.save_to_path(path);
    lvl.lvl.descr = 0; /* prevent destructor from trying to free 'input' */
}

static void get_name(void)
{
    printf("%.*s", lvl.lvl.name_len, lvl.lvl.name);
}

static void set_name(void)
{
    _get_input();
    if (input_len > 255) input_len = 255;
    lvl.lvl.name_len = input_len;
    memcpy(lvl.lvl.name, input, input_len);
    lvl.save_to_path(path);
}

static void get_type(void)
{
    printf("%d", lvl.lvl.type);
}

static void get_parent_id(void)
{
    printf("%d", lvl.lvl.parent_id);
}

static void get_version(void)
{
    printf("%d", lvl.lvl.version);
}

static void get_revision(void)
{
    printf("%d", lvl.lvl.revision);
}

static void get_allow_derivatives(void)
{
    printf("%d", lvl.lvl.allow_derivatives);
}

static void set_allow_derivatives(void)
{
    _get_input();
    lvl.lvl.allow_derivatives = (uint8_t)atoi(input);
    lvl.save_to_path(path);
}

static void get_visibility(void)
{
    printf("%d", lvl.lvl.visibility);
}

static void set_visibility(void)
{
    _get_input();
    lvl.lvl.visibility = (uint8_t)atoi(input);
    lvl.save_to_path(path);
}

static void get_community_id(void)
{
    printf("%u", lvl.lvl.community_id);
}

static void set_community_id(void)
{
    _get_input();
    lvl.lvl.community_id = (uint32_t)atoi(input);
    lvl.save_to_path(path);
}

static void flag_active(void)
{
    _get_input();
    uint64_t flag = (1ULL << atoi(input));
    printf("%u", lvl.lvl.flag_active(flag) ? 1 : 0);
}

static void get_gids(void)
{
    lvl.print_gids();
}

struct handler {
    const char *str;
    void (*fn)(void);
};

struct handler handlers[] = {
    {"--get-description", get_description},
    {"--set-description", set_description},
    {"--set-name", set_name},
    {"--get-name", get_name},
    {"--get-type", get_type},
    {"--get-parent-id", get_parent_id},
    {"--get-version", get_version},
    {"--get-revision", get_revision},
    {"--get-gids", get_gids},
    {"--get-allow-derivatives", get_allow_derivatives},
    {"--set-allow-derivatives", set_allow_derivatives},
    {"--get-visibility", get_visibility},
    {"--set-visibility", set_visibility},
    {"--get-community-id", get_community_id},
    {"--set-community-id", set_community_id},
    {"--flag-active", flag_active},
};
#define NUM_HANDLERS (sizeof(handlers)/sizeof(struct handler))

int
main(int argc, char **argv)
{
    if (argc == 3) {
        strcpy(path, argv[1]);

        if (!lvl.open_from_path(path)) {
            fprintf(stderr, "could not open file %s", path);
            exit(1);
        }

        /**
         * Get:
         * gids
         * type
         * version
         * revision
         * parent-id
         *
         * GetSet:
         * community_id
         * name
         * descr
         * allow_derivatives
         * locked
         *
         * flag_active
         **/

        for (int x=0; x<NUM_HANDLERS; x++) {
            if (strcmp(handlers[x].str, argv[2]) == 0) {
                (handlers[x].fn)();
                break;
            }
        }

        return 0;
    } else {
        fprintf(stderr, "why are we here\n");
        printf("usage: lvledit <path> <method>\n");
        for (int x=0; x<NUM_HANDLERS; x++) {
            printf("  %s\n", handlers[x].str);
        }

        return 1;
    }

    fprintf(stderr, "hello??\n");
    return 2;
}
