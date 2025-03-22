#include "storage.h"
#include "tms/backend/print.h"
#include <SDL.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#ifdef __ANDROID__
    #include <SDL3_polyfills.h>
#endif

#ifdef TMS_BACKEND_WINDOWS
    #include <direct.h>
    #define mkdir(dirname, ...) _mkdir(dirname)
    #define create_dir(dirname, ...) _create_dir(dirname, 0)
#else
    #include <pwd.h>
    #include <sys/stat.h>
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
        strcpy(path, getenv("USERPROFILE"));
        strcat(path, "\\Principia");
#else
        struct passwd *pw = getpwuid(getuid());
        strcpy(path, pw->pw_dir);
        strcat(path, "/.principia");
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
        char* exedir = SDL_GetBasePath();
        strcpy(path, exedir);
        strcat(path, "cache");
    } else { // System
#ifdef TMS_BACKEND_WINDOWS
        strcpy(path, getenv("LOCALAPPDATA"));
        strcat(path, "\\Principia");
#else
        const char* xdg_cache_home = getenv("XDG_CACHE_HOME");
        if (!xdg_cache_home) {
            strcpy(path, getenv("HOME"));
            strcat(path, "/.cache/principia");
        } else {
            strcpy(path, xdg_cache_home);
            strcat(path, "/principia");
        }
#endif
    }

    cache_path = path;
    return cache_path;
#endif
}

void tms_storage_create_dirs(void)
{
    char tmp[1024];
    static const char *s_dirs[]={
        "",
        "/lvl", "/lvl/local",
        "/pkg", "/pkg/local",
        "/sav"
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
