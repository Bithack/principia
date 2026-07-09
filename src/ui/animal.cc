#include "animal.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"
#include "world.hh"

namespace UiAnimal {
	static bool do_open = false;

    void change_animal(int animal) {
        entity* e = G->selection.e;

        if (e && e->g_id == O_ANIMAL) {
            W->add_action(e->id, ACTION_SET_ANIMAL_TYPE, UINT_TO_VOID((uint32_t)animal));

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }

    void open() {
        do_open = true;
    }

    void layout() {
        handle_do_open(&do_open, "animal_type");

        if (ImGui::BeginPopup("animal_type", POPUP_FLAGS)) {
            for (int i = 0; i < NUM_ANIMAL_TYPES; ++i)
                if (ImGui::MenuItem(animal_data[i].name))
                    change_animal(i);

            ImGui::EndPopup();
        }
    }
}
