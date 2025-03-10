
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <tms/core/project.h>
#include <tms/core/event.h>
#include <tms/core/tms.h>

#include <tms/backend/opengl.h>

#ifdef TMS_BACKEND_WINDOWS

#include <windows.h>
#include <windowsx.h>

#include "shlwapi.h"

char *_tmp[]={0,0};
static HANDLE pipe_h;
static uint8_t buf[512];

#else


static char *_args[2] = {0,0};
static int pipe_h;
static char buf[1024];

#endif

int _pipe_listener(void *p)
{
#ifdef TMS_BACKEND_WINDOWS
    DWORD num_read;

    while (ConnectNamedPipe(pipe_h, 0) || GetLastError() == ERROR_PIPE_CONNECTED) {
        tms_infof("Client connected, reading...");

        if (ReadFile(pipe_h, buf, 511, &num_read, 0)) {
            tms_infof("read %u bytes:", num_read);
            buf[num_read] = '\0';
            tms_infof("%s", buf);

            _tmp[1] = (char*)buf;

            tproject_set_args(2, _tmp);
        } else
            tms_infof("error reading from pipe: %d", GetLastError());

        FlushFileBuffers(pipe_h);
        DisconnectNamedPipe(pipe_h);
    }

    tms_infof("ConnectNamedPipe returned false, quitting");

    CloseHandle(pipe_h);

    return T_OK;

#elif defined(TMS_BACKEND_HAIKU) || defined(TMS_BACKEND_EMSCRIPTEN)

    // Unimplemented
    return 0;

#else

    ssize_t sz;

    while (1) {
        tms_infof("attempting to open /tmp/principia.run O_RDONLY");
        while ((pipe_h = open("/tmp/principia.run", O_RDONLY)) == -1) {
            if (errno != EINTR)
                return 1;
        }

        while ((sz = read(pipe_h, buf, 1023)) > 0) {
            //tms_infof("read %d bytes", sz);

            if (sz > 0) {
                buf[sz] = '\0';
                _args[1] = buf;
                tproject_set_args(2, _args);
            }
        }

        close(pipe_h);
    }

    tms_infof("Pipe listener EXITING");
#endif
}

void setup_pipe(int argc, char **argv)
{
#ifdef TMS_BACKEND_WINDOWS

    pipe_h = CreateNamedPipe(
            L"\\\\.\\pipe\\principia-process",
            PIPE_ACCESS_INBOUND,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1,
            128,
            128,
            0,
            0
            );

    if (pipe_h == INVALID_HANDLE_VALUE) {
        /* could not create named pipe */
        tms_infof("Forwarding arguments through pipe...");

        while (1) {
            pipe_h = CreateFile(
                    L"\\\\.\\pipe\\principia-process",
                    GENERIC_WRITE,
                    0,
                    0,
                    OPEN_EXISTING,
                    0,
                    0
                );

            if (pipe_h != INVALID_HANDLE_VALUE)
                break;

            if (GetLastError() != ERROR_PIPE_BUSY) {
                tms_errorf("error opening pipe");
                exit(1);
            }

            tms_infof("Waiting for pipe...");
            if (!WaitNamedPipe((LPCWSTR)pipe_h, 3000))
                tms_errorf("Failed, waited too long.");
        }

        DWORD dwMode = PIPE_READMODE_MESSAGE;
        SetNamedPipeHandleState(
                pipe_h,
                &dwMode,
                0,0);

        if (argc > 1) {
            DWORD written;
            if (!(WriteFile(pipe_h, argv[1], strlen(argv[1]), &written, 0))) {
                tms_errorf("error writing to pipe");
            }

            tms_infof("done");
        }

        CloseHandle(pipe_h);

        /* bring the window to the front */
        HWND h = FindWindow(NULL, L"Principia");
        SetForegroundWindow(h);
        exit(0);
    } else {
        /* we've created the named pipe */
        tms_infof("Created named pipe, starting listener thread.");
        SDL_CreateThread(_pipe_listener, "_pipe_listener", 0);
    }

#elif defined(TMS_BACKEND_HAIKU) || defined(TMS_BACKEND_EMSCRIPTEN)

    // Unimplemented


#else
    int status = mkfifo("/tmp/principia.run", S_IWUSR | S_IRUSR);
    int skip_pipe = 0;

    if (status == 0) {
        tms_infof("Created fifo");
    } else {
        if (errno != EEXIST) {
            tms_errorf("could not create fifo pipe!");
            skip_pipe = 1;
        }
    }

    if (!skip_pipe) {
        if ((pipe_h = open("/tmp/principia.run", O_WRONLY | O_NONBLOCK)) == -1) {
            if (errno != ENXIO) {
                skip_pipe = 1;
                tms_infof("error: %s", strerror(errno));
            }
        } else {
            if (argc > 1) {
                /* open the fifo for writing instead */
                tms_infof("sending arg: %s", argv[1]);

                write(pipe_h, argv[1], strlen(argv[1]));
            } else {
                tms_infof("principia already running");
            }

            close(pipe_h);
            exit(0);
        }
    }

    if (!skip_pipe) {
        tms_infof("Starting fifo listener thread");
        SDL_CreateThread(_pipe_listener, "_pipe_listener", 0);
    }
#endif
}
