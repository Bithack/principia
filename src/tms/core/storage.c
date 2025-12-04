#include "storage.h"
#include "tms/backend/print.h"
#include <SDL.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __ANDROID__
    #include <SDL3_polyfills.h>
#endif

#ifdef TMS_BACKEND_WINDOWS
    #include <direct.h>
    #include <windows.h>
    #define mkdir(dirname, ...) _mkdir(dirname)
    #define create_dir(dirname, ...) _create_dir(dirname, 0)
#else
    #include <dirent.h>
    #include <pwd.h>
    #define create_dir _create_dir
#endif

static char *_storage_path = 0;

static bool _storage_portable = false;

void tms_storage_set_portable(bool portable)
{
    _storage_portable = portable;
}

bool
_create_dir(const char *path, mode_t mode)
{
    if (mkdir(path, mode) != 0) {
        switch (errno) {
            case EACCES:
                tms_errorf("We lack permissions to create folder %s", path);
                return false;

            case EEXIST:
                /* this should not be considered an error */
                return true;

            case ENAMETOOLONG:
                tms_errorf("Name of directory %s is too long.", path);
                return false;

            case ENOENT:
                tms_errorf("Parent directory for %s not found.", path);
                return false;

            default:
                tms_errorf("An unknown error occurs when attempting to create directory %s (%d)", path, errno);
                return false;
        }
    }

    return true;
}

const char *tms_storage_path(void)
{
#ifdef __ANDROID__
    return SDL_AndroidGetExternalStoragePath();
#elif defined(__EMSCRIPTEN__)
    return SDL_GetPrefPath("Bithack", "Principia");
#elif defined(SCREENSHOT_BUILD)
    return "storage";
#else
    if (_storage_path)
        return _storage_path;


    char *path = (char *)malloc(1024);

    if (_storage_portable) { // Portable
        char* exedir = SDL_GetBasePath();
        strcpy(path, exedir);
        strcat(path, "userdata");
    } else { // System
#ifdef TMS_BACKEND_WINDOWS
        snprintf(path, 1024, "%s\\Principia", getenv("APPDATA"));
#else
        const char *xdg = getenv("XDG_DATA_HOME");
        if (!xdg)
            snprintf(path, 1024, "%s/.local/share/principia", getenv("HOME"));
        else
            snprintf(path, 1024, "%s/principia", xdg);
#endif
    }

    _storage_path = path;

    tms_infof("Storage path: %s", path);
    return _storage_path;
#endif
}

const char *tms_storage_cache_path(void)
{
#ifdef __ANDROID__
    return SDL_GetAndroidCachePath();
#elif defined(__EMSCRIPTEN__)
    return SDL_GetPrefPath("Bithack", "Principia");
#elif defined(SCREENSHOT_BUILD)
    return "/tmp/principia_cache";
#else
    static char *cache_path = 0;

    if (cache_path)
        return cache_path;

    char *path = (char *)malloc(1024);

    if (_storage_portable) { // Portable
        snprintf(path, 1024, "%s/cache", SDL_GetBasePath());
    } else { // System
#ifdef TMS_BACKEND_WINDOWS
        snprintf(path, 1024, "%s\\Principia", getenv("LOCALAPPDATA"));
#else
        const char *xdg = getenv("XDG_CACHE_HOME");
        if (!xdg)
            snprintf(path, 1024, "%s/.cache/principia", getenv("HOME"));
        else
            snprintf(path, 1024, "%s/principia", xdg);
#endif
    }

    cache_path = path;
    return cache_path;
#endif
}

static void migrate_principia_data();

void tms_storage_create_dirs(void)
{
    migrate_principia_data();

    char tmp[1024];
    static const char *s_dirs[]={
        "",
        "/levels",
        "/saves"
    };

    for (int x=0; x<sizeof(s_dirs)/sizeof(const char*); x++) {
        sprintf(tmp, "%s%s", tms_storage_path(), s_dirs[x]);
        create_dir(tmp, S_IRWXU | S_IRWXG | S_IRWXO);
    }

    static const char *c_dirs[]={
        "",
        "/cache", "/cache/db", "/cache/local", "/cache/main", "/cache/sav",
        "/lvl", "/lvl/db",
        "/pkg", "/pkg/db"
    };
    for (int x=0; x<sizeof(c_dirs)/sizeof(const char*); x++) {
        sprintf(tmp, "%s%s", tms_storage_cache_path(), c_dirs[x]);
        create_dir(tmp, S_IRWXU | S_IRWXG | S_IRWXO);
    }
}

// Storage migration code

static int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static int dir_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static void move_matching_files(const char *srcdir, const char *dstdir, const char *ext) {
#ifdef TMS_BACKEND_WINDOWS
    char pattern[MAX_PATH];
    snprintf(pattern, sizeof(pattern), "%s\\*.%s", srcdir, ext);

    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile(pattern, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        char src[MAX_PATH], dst[MAX_PATH];
        snprintf(src, sizeof(src), "%s\\%s", srcdir, ffd.cFileName);
        snprintf(dst, sizeof(dst), "%s\\%s", dstdir, ffd.cFileName);

        rename(src, dst);
    } while (FindNextFile(hFind, &ffd) != 0);

    FindClose(hFind);
#else
    DIR *dir = opendir(srcdir);
    if (!dir) return;

    struct dirent *ent;
    while ((ent = readdir(dir))) {
        if (ent->d_type == DT_DIR) continue;

        const char *fname = ent->d_name;
        const char *dot = strrchr(fname, '.');
        if (!dot || strcmp(dot + 1, ext) != 0) continue;

        char src[1024], dst[1024];
        snprintf(src, sizeof(src), "%s/%s", srcdir, fname);
        snprintf(dst, sizeof(dst), "%s/%s", dstdir, fname);

        rename(src, dst);
    }
    closedir(dir);
#endif
}

static void migrate_principia_data() {
    char oldpath[1024], newpath[1024], oldpath_old[1024],
        old_levels[1024], new_levels[1024], old_saves[1024], new_saves[1024];

    if (_storage_portable) {
        snprintf(oldpath, sizeof(oldpath), "userdata");
        snprintf(newpath, sizeof(newpath), "userdata");
    } else {
#ifdef TMS_BACKEND_WINDOWS
        // Old: %USERPROFILE%/Principia
        snprintf(oldpath, sizeof(oldpath), "%s\\Principia", getenv("USERPROFILE"));
        // New: %APPDATA%/Principia
        snprintf(newpath, sizeof(newpath), "%s\\Principia", getenv("APPDATA"));

#elif defined(TMS_BACKEND_LINUX) && !defined(SCREENSHOT_BUILD)
        // Old: ~/.principia
        snprintf(oldpath, sizeof(oldpath), "%s/.principia", getenv("HOME"));
        // New: $XDG_DATA_HOME/principia or ~/.local/share/principia
        const char *xdg = getenv("XDG_DATA_HOME");
        if (!xdg)
            snprintf(newpath, sizeof(newpath), "%s/.local/share/principia", getenv("HOME"));
        else
            snprintf(newpath, sizeof(newpath), "%s/principia", xdg);

#elif defined(TMS_BACKEND_ANDROID)
        snprintf(oldpath, sizeof(oldpath), "%s", SDL_AndroidGetExternalStoragePath());
        snprintf(newpath, sizeof(newpath), "%s", SDL_AndroidGetExternalStoragePath());
#else
        // No migration for this platform
        return;
#endif
    }

    snprintf(old_levels, sizeof(old_levels), "%s/levels", oldpath);

    // Return if old path does not exist and new path already exists (unless both paths are the same)
    if (!dir_exists(oldpath) && dir_exists(newpath))
        return;

    if (strcmp(oldpath, newpath) == 0 && dir_exists(old_levels))
        return;

    // We should migrate now
    printf("Migrating Principia data from %s -> %s\n", oldpath, newpath);

    // Rename old directory to [...]_old
    snprintf(oldpath_old, sizeof(oldpath_old), "%s_old", oldpath);
    rename(oldpath, oldpath_old);
    strncat(oldpath, "_old", sizeof(oldpath)-1);

    snprintf(old_levels, sizeof(old_levels), "%s/lvl/local", oldpath);
    snprintf(new_levels, sizeof(new_levels), "%s/levels", newpath);

    snprintf(old_saves, sizeof(old_saves), "%s/sav", oldpath);
    snprintf(new_saves, sizeof(new_saves), "%s/saves", newpath);

    // Create dirs for new location
    create_dir(newpath, S_IRWXU | S_IRWXG | S_IRWXO);
    create_dir(new_levels, S_IRWXU | S_IRWXG | S_IRWXO);
    create_dir(new_saves, S_IRWXU | S_IRWXG | S_IRWXO);

    // Move individual files
    struct {
        const char *src;
        const char *dst;
    } files[] = {
        {"c", "cookie_jar"},
        {"community_host.txt", "community_host.txt"},
        {"data.bin", "data.bin"},
        {"run.log", "run.log"},
        {"settings.ini", "settings.ini"},
        {"lvl/local/.autosave", "levels/.autosave"},
        {NULL, NULL}
    };
    for (int i = 0; files[i].src; i++) {
        char src[1024], dst[1024];
        snprintf(src, sizeof(src), "%s/%s", oldpath, files[i].src);
        snprintf(dst, sizeof(dst), "%s/%s", newpath, files[i].dst);

        // Only move if source exists and there is nothing in the new location
        if (file_exists(src) && !file_exists(dst))
            rename(src, dst);
    }

    // Move levels, multi-select objects and puzzle solutions
    move_matching_files(old_levels, new_levels, "plvl");
    move_matching_files(old_levels, new_levels, "pobj");
    move_matching_files(old_levels, new_levels, "psol");

    move_matching_files(old_saves, new_saves, "psav");
}
