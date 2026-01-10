#include "animal.hh"
#include "ui_imgui.hh"

namespace UiAnimal {
	static bool do_open = false;

    void open() {
        do_open = true;
    }

    void layout() {
        handle_do_open(&do_open, "Animal type");
        ImGui::SetNextWindowSize(ImVec2(200, .0));
        if (ImGui::BeginPopupModal("Animal type", REF_TRUE, MODAL_FLAGS)) {
            for (int i = 0; i < NUM_ANIMAL_TYPES; ++i) {
                if (ImGui::MenuItem(animal_data[i].name)) {
                    entity* e = G->selection.e;

                    if (e && e->g_id == O_ANIMAL) {
                        W->add_action(e->id, ACTION_SET_ANIMAL_TYPE, UINT_TO_VOID((uint32_t)i));

                        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
                        P.add_action(ACTION_RESELECT, 0);
                    }
                }
            }

            ImGui::EndPopup();
        }
    }
}
