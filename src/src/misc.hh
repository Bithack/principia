#pragma once

#include <sys/stat.h>
#include <time.h>
#include <vector>
#include <cstdint>

#ifndef _NO_TMS
#include <tms/math/vector.h>
#endif

#if defined(TMS_BACKEND_ANDROID)
#include <SDL.h>

#define _FILE void
#define FILE_IN_ASSET(x) int file_in_asset = (x);
#define _fopen(x,y) (FILE*)(file_in_asset ? (void*)SDL_RWFromFile(x,y) : (void*)fopen(x,y))
#define _fclose(x) (file_in_asset ? SDL_RWclose((SDL_RWops*)x) : fclose((FILE*)x))
#define _fread(x,y,z,a) (file_in_asset ? SDL_RWread((SDL_RWops*)a,x,y,z) : fread(x,y,z,(FILE*)a))
#define _fseek(x,y,z) (file_in_asset ? SDL_RWseek((SDL_RWops*)x,y,z) : fseek((FILE*)x,y,z))
#define _ftell(x) (file_in_asset ? SDL_RWtell((SDL_RWops*)x) : ftell((FILE*)x))

#else
#define _FILE FILE
#define FILE_IN_ASSET(x)
#define _fopen fopen
#define _fclose fclose
#define _fread fread
#define _fseek fseek
#define _ftell ftell
#endif

#define DOUBLETIME_TO_INT64(x) (int64_t)(x * 1000000.0)
#define VOID_TO_UINT64(x) (uint64_t)(uintptr_t)(x)
#define VOID_TO_UINT32(x) (uint32_t)(uintptr_t)(x)
#define VOID_TO_UINT8(x) (uint8_t)(uintptr_t)(x)
#define VOID_TO_INT(x) (int)(intptr_t)(x)
#define INT_TO_VOID(x) (void*)(intptr_t)(x)
#define UINT_TO_VOID(x) (void*)(uintptr_t)(x)

#define tvec2b(X) tvec2f(X.x, X.y)
#define b2Vec2t(X) b2Vec2(X.x, X.y)

#define ACTIVE_MISC_WIDGET_COLOR 1.0f, 1.5f, 1.0f, 1.0f
#define FPS_GRAPH_OUTLINE_COLOR 1.0f, 0.33f, 0.33f, 0.66f
#define FPS_GRAPH_COLOR 0.0f, 0.66f, 0.0f, 0.66f
#define FPS_GRAPH_COLOR_END 0.33f, 1.0f, 0.33f, 1.0f

#define MENU_WHITE_F (tvec3){221.f/255.f, 221.f/255.f, 221.5/255.f}
#define MENU_BLACK_F (tvec3){54.f/255.f, 54.f/255.f, 54.f/255.f}
#define MENU_GRAY_F  (tvec3){102.f/255.f, 102.f/255.f, 102.f/255.f}

#define MENU_WHITE_FI 221.f/255.f, 221.f/255.f, 221.5/255.f
#define MENU_BLACK_FI 54.f/255.f, 54.f/255.f, 54.f/255.f
#define MENU_GRAY_FI  102.f/255.f, 102.f/255.f, 102.f/255.f

#define MENU_WHITE 221, 221, 221
#define MENU_BLACK 54, 54, 54
#define MENU_GRAY  102, 102, 102

#ifndef _NO_TMS
static tvec4 TV_BLACK = {0.f,  0.f,  0.f,  1.f};
static tvec4 TV_WHITE = {1.f,  1.f,  1.f,  1.f};
static tvec4 TV_RED   = {1.f,  0.f,  0.f,  1.f};

static tvec3 TV_HP_RED        = {1.f,  .33f, .33f};
static tvec3 TV_HP_GREEN      = {.33f, 1.f,  .33f};
static tvec3 TV_HP_GRAY       = {.5f,  .5f,  .5f};
static tvec3 TV_HP_LGRAY      = {.7f,  .7f,  .7f};
static tvec3 TV_HP_ZOMBIE     = {.4f,  .4f,  .4f};
static tvec3 TV_HP_COMPRESSOR = {.37f, .97f, .97f};

static tvec3 TV_MENU_WHITE = MENU_WHITE_F;
static tvec3 TV_MENU_BLACK = MENU_BLACK_F;
static tvec3 TV_MENU_GRAY  = MENU_GRAY_F;
#endif

inline bool
file_exists(const char *path)
{
    struct stat s;
    int i = stat(path, &s);

    return (i == 0);
}

time_t get_mtime(const char *path);

inline uint8_t
tpixel_mat_to_chunk_mat(uint8_t mat)
{
    return mat + 1;
}

inline uint8_t
chunk_mat_to_tpixel_mat(uint8_t mat)
{
    return mat - 1;
}

std::vector<char*> p_split(const char *str, size_t len, const char *delim);

inline int
highscore_offset(uint32_t community_id)
{
    int offset = 0;

    switch (community_id % 10) {
        case 0: offset = 1; break;
        case 1: offset = 2; break;
        case 2: offset = 4; break;
        case 3: offset = 0; break;
        case 4: offset = 2; break;
        case 5: offset = 1; break;
        case 6: offset = 2; break;
        case 7: offset = 2; break;
        case 9: offset = 4; break;
    }

    return offset;
}

#define GAMMA_CORRECTF(f) (_tms.gamma_correct ? powf(f, 2.2) : f)

#define VEC2_INLINE(v) (v).x, (v).y
#define VEC3_INLINE(v) (v).x, (v).y, (v).z
