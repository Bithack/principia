/*
    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the Perl Artistic License, available in COPYING.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "config.h"
#include "common.h"
#include "output.h"
#include "ctrlmode.h"
#include "instrum.h"
#include "playmidi.h"

static void ctl_refresh(void);
static void ctl_total_time(int tt);
static void ctl_master_volume(int mv);
static void ctl_file_name(char *name);
static void ctl_current_time(int ct);
static void ctl_note(int v);
static void ctl_program(int ch, int val);
static void ctl_volume(int channel, int val);
static void ctl_expression(int channel, int val);
static void ctl_panning(int channel, int val);
static void ctl_sustain(int channel, int val);
static void ctl_pitch_bend(int channel, int val);
static void ctl_reset(void);
static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, char *fmt, ...);

/**********************************/
/* export the interface functions */

#define ctl sdl_control_mode

ControlMode ctl= 
{
  "SDL interface", 's',
  1,0,0,
  ctl_open,NULL, ctl_close, ctl_read, cmsg,
  ctl_refresh, ctl_reset, ctl_file_name, ctl_total_time, ctl_current_time, 
  ctl_note, 
  ctl_master_volume, ctl_program, ctl_volume, 
  ctl_expression, ctl_panning, ctl_sustain, ctl_pitch_bend
};

static int ctl_open(int using_stdin, int using_stdout)
{
  ctl.opened=1;
  return 0;
}

static void ctl_close(void)
{ 
  ctl.opened=0;
}

static int ctl_read(int32 *valp)
{
  return RC_NONE;
}

static int cmsg(int type, int verbosity_level, char *fmt, ...)
{
#ifdef GREGS_DEBUG
  va_list ap;
  int flag_newline = 1;
  if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
      ctl.verbosity<verbosity_level-1)
    return 0;
  if (*fmt == '~')
    {
      flag_newline = 0;
      fmt++;
    }
  va_start(ap, fmt);
  if (!ctl.opened)
    {
      vfprintf(stderr, fmt, ap);
      if (flag_newline) fprintf(stderr, "\n");
    }
  else
    {
      vfprintf(stderr, fmt, ap);
      if (flag_newline) fprintf(stderr, "\n");
    }
  va_end(ap);
  if (!flag_newline) fflush(stderr);
  return 0;
#else
  va_list ap;
  if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
      ctl.verbosity<verbosity_level)
    return 0;
  va_start(ap, fmt);
  SDL_vsnprintf(timidity_error, TIMIDITY_ERROR_SIZE, fmt, ap);
  va_end(ap);
  return 0;
#endif
}

static void ctl_refresh(void) { }

static void ctl_total_time(int tt) {}

static void ctl_master_volume(int mv) {}

static void ctl_file_name(char *name) {}

static void ctl_current_time(int ct) {}

static void ctl_note(int v) {}

static void ctl_program(int ch, int val) {}

static void ctl_volume(int channel, int val) {}

static void ctl_expression(int channel, int val) {}

static void ctl_panning(int channel, int val) {}

static void ctl_sustain(int channel, int val) {}

static void ctl_pitch_bend(int channel, int val) {}

static void ctl_reset(void) {}
