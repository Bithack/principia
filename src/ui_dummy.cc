#include "ui.hh"

// Dummy dialog backend for minimal builds (e.g. the screenshotter build)

#ifdef NO_UI

void ui::init(){};
void ui::open_dialog(int num, void *data/*=0*/){}
void ui::open_sandbox_tips(){};
void ui::emit_signal(int num, void *data/*=0*/){};
void ui::quit(){};
void ui::set_next_action(int action_id){};
void ui::open_error_dialog(const char *error_msg){};
void
ui::confirm(const char *text,
        const char *button1, principia_action action1,
        const char *button2, principia_action action2,
        const char *button3/*=0*/, principia_action action3/*=ACTION_IGNORE*/,
        struct confirm_data _confirm_data/*=none*/
        )
{
    P.add_action(action1.action_id, 0);
}
void ui::alert(const char*, uint8_t/*=ALERT_INFORMATION*/) {};
void ui::render(){};

#endif
