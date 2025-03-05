#pragma once

#include "text.hh"
#include "tms/bindings/cpp/cpp.hh"

#define TEXTBUFFER_MAX 512

class textbuffer
{
  public:
    static void _init();
    static void reset();
    static void upload();
    static tms::entity *get_entity();
    static tms::entity *get_entity2();

    static void add_char(glyph *gl, float x, float y, float z, float r, float g, float b, float a, float w, float h);
    static void add_char2(glyph *gl, float x, float y, float z, float r, float g, float b, float a, float w, float h);
    static void add_text(p_text *text, float scale);
    static void add_text(const char *text, p_font *font, float x, float y, float z, float r, float g, float b, float a, float scale, uint8_t horizontal_align=ALIGN_CENTER, uint8_t vertical_align=ALIGN_CENTER, bool bg=false);
    static void add_text2(const char *text, p_font *font, float x, float y, float z, float r, float g, float b, float a, float scale, uint8_t horizontal_align=ALIGN_CENTER, uint8_t vertical_align=ALIGN_CENTER);
    static void add_bg(float x, float y, float z, float r, float g, float b, float a, float w, float h);
};
