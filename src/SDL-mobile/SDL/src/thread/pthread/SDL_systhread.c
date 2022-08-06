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

#include <pthread.h>

#if HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif

#include <signal.h>
#ifdef __LINUX__
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
extern int pthread_setname_np (pthread_t __target_thread, __const char *__name) __THROW __nonnull ((2));
#endif

#include "SDL_platform.h"
#include "SDL_thread.h"
#include "../SDL_thread_c.h"
#include "../SDL_systhread.h"
#ifdef __ANDROID__
#include "../../core/android/SDL_android.h"
#endif

/* List of signals to mask in the subthreads */
static const int sig_list[] = {
    SIGHUP, SIGINT, SIGQUIT, SIGPIPE, SIGALRM, SIGTERM, SIGCHLD, SIGWINCH,
    SIGVTALRM, SIGPROF, 0
};


static void *
RunThread(void *data)
{
#ifdef __ANDROID__
    Android_JNI_SetupThread();
#endif
    SDL_RunThread(data);
    return NULL;
}

int
SDL_SYS_CreateThread(SDL_Thread * thread, void *args)
{
    pthread_attr_t type;

    /* Set the thread attributes */
    if (pthread_attr_init(&type) != 0) {
        SDL_SetError("Couldn't initialize pthread attributes");
        return (-1);
    }
    pthread_attr_setdetachstate(&type, PTHREAD_CREATE_JOINABLE);

    /* Create the thread and go! */
    if (pthread_create(&thread->handle, &type, RunThread, args) != 0) {
        SDL_SetError("Not enough resources to create thread");
        return (-1);
    }

    return (0);
}

void
SDL_SYS_SetupThread(const char *name)
{
    int i;
    sigset_t mask;

    if (name != NULL) {
#if ( (__MACOSX__ && (MAC_OS_X_VERSION_MAX_ALLOWED >= 1060)) || \
      (__IPHONEOS__ && (__IPHONE_OS_VERSION_MAX_ALLOWED >= 30200)) )
        if (pthread_setname_np != NULL) { pthread_setname_np(name); }
#elif HAVE_PTHREAD_SETNAME_NP
        pthread_setname_np(pthread_self(), name);
#elif HAVE_PTHREAD_SET_NAME_NP
        pthread_set_name_np(pthread_self(), name);
#endif
    }

    /* Mask asynchronous signals for this thread */
    sigemptyset(&mask);
    for (i = 0; sig_list[i]; ++i) {
        sigaddset(&mask, sig_list[i]);
    }
    pthread_sigmask(SIG_BLOCK, &mask, 0);

#ifdef PTHREAD_CANCEL_ASYNCHRONOUS
    /* Allow ourselves to be asynchronously cancelled */
    {
        int oldstate;
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldstate);
    }
#endif
}

SDL_threadID
SDL_ThreadID(void)
{
    return ((SDL_threadID) pthread_self());
}

int
SDL_SYS_SetThreadPriority(SDL_ThreadPriority priority)
{
#ifdef __LINUX__
    int value;

    if (priority == SDL_THREAD_PRIORITY_LOW) {
        value = 19;
    } else if (priority == SDL_THREAD_PRIORITY_HIGH) {
        value = -20;
    } else {
        value = 0;
    }
    if (setpriority(PRIO_PROCESS, syscall(SYS_gettid), value) < 0) {
        /* Note that this fails if you're trying to set high priority
           and you don't have root permission. BUT DON'T RUN AS ROOT!
         */
        SDL_SetError("setpriority() failed");
        return -1;
    }
    return 0;
#else
    struct sched_param sched;
    int policy;
    pthread_t thread = pthread_self();

    if (pthread_getschedparam(thread, &policy, &sched) < 0) {
        SDL_SetError("pthread_getschedparam() failed");
        return -1;
    }
    if (priority == SDL_THREAD_PRIORITY_LOW) {
        sched.sched_priority = sched_get_priority_min(policy);
    } else if (priority == SDL_THREAD_PRIORITY_HIGH) {
        sched.sched_priority = sched_get_priority_max(policy);
    } else {
        int min_priority = sched_get_priority_min(policy);
        int max_priority = sched_get_priority_max(policy);
        sched.sched_priority = (min_priority + (max_priority - min_priority) / 2);
    }
    if (pthread_setschedparam(thread, policy, &sched) < 0) {
        SDL_SetError("pthread_setschedparam() failed");
        return -1;
    }
    return 0;
#endif /* linux */
}

void
SDL_SYS_WaitThread(SDL_Thread * thread)
{
    pthread_join(thread->handle, 0);
}

/* vi: set ts=4 sw=4 expandtab: */
