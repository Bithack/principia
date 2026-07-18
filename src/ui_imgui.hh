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
namespace UiLevelProperties { void open(); void layout(); void reload_border_sizes(); }
namespace UiSave            { void open(bool copy_flag); void layout(); }
namespace UiNewLevel        { void open(); void layout(); }
namespace UiFrequency       { void open(bool is_range); void layout(); }
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
namespace UiEmitter         { void open(); void layout(); }
namespace UiCommandPad      { void open(); void layout(); }
namespace UiPkgLvlSelector  { void open(); void layout(); }
namespace UiEventListener   { void open(); void layout(); }
namespace UiResource        { void open(); void layout(); }
namespace UiItem            { void open(); void layout(); }
namespace UiShapeExtruder   { void open(); void layout(); }
namespace UiCursorField     { void open(); void layout(); }
namespace UiSetFaction      { void open(); void layout(); }
namespace UiKeyListener     { void open(); void layout(); }
namespace UiPublish         { void open(); void layout(); }
namespace UiPublished       { void open(); void layout(); }
namespace UiTimer           { void open(); void layout(); }
namespace UiCommunity       { void open(); void layout(); }
namespace UiConfirmQuit     { void open(); void layout(); }
namespace UiCamTargeter     { void open(); void layout(); }
namespace UiVendor          { void open(); void layout(); }
namespace UiFXEmitter       { void open(); void layout(); }
namespace UiPrompt          { void open(); void layout(); }
namespace UiPromptSettings  { void open(); void layout(); }
namespace UiSoundManager    { void open(); void layout(); }
namespace UiSequencer       { void open(); void layout(); }
namespace UiExport          { void open(); void layout(); }
namespace UiDigitalDisplay  { void open(); void layout(); }
namespace UiFactory         { void open(); void layout(); }
namespace UiSfxEmitter      { void open(); void layout(); }
namespace UiSfxEmitterLegacy { void open(); void layout(); }
