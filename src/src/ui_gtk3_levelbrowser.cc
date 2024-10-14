#include "ui_gtk3_levelbrowser.hh"

#if !defined(GTK3_LEVEL_BROWSER_DISABLE) && defined(TMS_BACKEND_PC) && !defined(NO_UI) && !defined(TMS_BACKEND_EMSCRIPTEN)

// TODO maybe vendor?
#include <nlohmann/json.hpp>
using json = nlohmann::json;

GtkCommunityDialog::GtkCommunityDialog() {

}

GtkCommunityDialog::~GtkCommunityDialog() {

}

#endif

