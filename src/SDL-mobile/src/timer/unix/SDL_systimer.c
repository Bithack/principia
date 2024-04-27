/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "SDL_config.h"

#ifdef SDL_TIMER_UNIX

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#include "SDL_timer.h"

/* The clock_gettime provides monotonous time, so we should use it if
   it's available. The clock_gettime function is behind ifdef
   for __USE_POSIX199309
   Tommi Kyntola (tommi.kyntola@ray.fi) 27/09/2005
*/
#include <time.h>

/* The first ticks value of the application */
static struct timeval start;


void
SDL_StartTicks(void)
{
    /* Set first ticks value */
    gettimeofday(&start, NULL);
}

Uint32
SDL_GetTicks(void)
{
    Uint32 ticks;
    struct timeval now;

    gettimeofday(&now, NULL);
    ticks =
        (now.tv_sec - start.tv_sec) * 1000 + (now.tv_usec -
                                              start.tv_usec) / 1000;
    return (ticks);
}

Uint64
SDL_GetPerformanceCounter(void)
{
    Uint64 ticks;
    struct timeval now;

    gettimeofday(&now, NULL);
    ticks = now.tv_sec;
    ticks *= 1000000;
    ticks += now.tv_usec;
    return (ticks);
}

Uint64
SDL_GetPerformanceFrequency(void)
{
    return 1000000;
}

void
SDL_Delay(Uint32 ms)
{
    int was_error;

    struct timespec elapsed, tv;

    /* Set the timeout interval */
    elapsed.tv_sec = ms / 1000;
    elapsed.tv_nsec = (ms % 1000) * 1000000;

    do {
        errno = 0;

        tv.tv_sec = elapsed.tv_sec;
        tv.tv_nsec = elapsed.tv_nsec;
        was_error = nanosleep(&tv, &elapsed);
    } while (was_error && (errno == EINTR));
}

#endif /* SDL_TIMER_UNIX */

/* vi: set ts=4 sw=4 expandtab: */
