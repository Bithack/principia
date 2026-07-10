#pragma once

#include "entity.hh"
#include "game.hh"
#include "main.hh"
#include "ui.hh"

enum class MessageType {
    Message,
    Error,
    LevelInfo
};

namespace UiSandboxMenu     { void open(); void layout(); }
namespace UiPlayMenu        { void open(); void layout(); }
namespace UiLevelManager    { void open(); void layout(); }
namespace UiLogin           { void open(); void layout(); void complete_login(int signal); }
namespace UiMessage         { void open(const char* msg, MessageType typ = MessageType::Message); void layout(); }
namespace UiSettings        { void open(); void layout(); }
namespace UiLuaEditor       { void open(entity *e = G->selection.e); void layout(); }
namespace UiTips            { void open(); void layout(); }
namespace UiSandboxMode     { void open(); void layout(); }
namespace UiQuickadd        { void open(); void layout(); }
namespace UiSynthesizer     { void open(entity *e = G->selection.e); void layout(); }
namespace UiObjColorPicker  { void open(bool alpha = false, entity *e = G->selection.e); void layout(); }
namespace UiLevelProperties { void open(); void layout(); }
namespace UiSave            { void open(); void layout(); }
namespace UiNewLevel        { void open(); void layout(); }
namespace UiFrequency       { void open(bool is_range, entity *e = G->selection.e); void layout(); }
namespace UiConfirm         { void open(const char* text, const char* button1, principia_action action1, const char* button2, principia_action action2, const char* button3, principia_action action3, struct confirm_data  _confirm_data); void layout(); }
namespace UiAnimal          { void open(); void layout(); }
namespace UiRobot           { void open(); void layout(); }
namespace UiSticky          { void open(); void layout(); }
namespace UiTreasureChest   { void open(); void layout(); }
namespace UiPolygon         { void open(); void layout(); }
namespace UiRubber          { void open(); void layout(); }
namespace UiDecoration      { void open(); void layout(); }
namespace UiVariable        { void open(); void layout(); }
namespace UiJumper          { void open(); void layout(); }
