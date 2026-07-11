#include "jumper.hh"
#include "entity.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"

namespace UiJumper {
	static bool do_open = false;
	static float jumper_value = 1.0f;

	void apply_properties() {
		entity* e = G->selection.e;
		if (e && e->g_id == O_JUMPER) {
			e->properties[0].v.f = jumper_value;

			P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
			P.add_action(ACTION_RESELECT, 0);

			((jumper*)e)->update_color();

			ImGui::CloseCurrentPopup();
		}
	}

	void open() {
		do_open = true;

		entity* e = G->selection.e;
		if (e && e->g_id == O_JUMPER)
			jumper_value = e->properties[0].v.f;
	}

	void layout() {
		handle_do_open(&do_open, "Jumper");

		ImGui_CenterNextWindow();
		if (ImGui::BeginPopupModal("Jumper", REF_TRUE, MODAL_FLAGS)) {

			ImGui::SliderFloat("Value", &jumper_value, 0.0f, 1.0f, "%.4f",
				ImGuiSliderFlags_AlwaysClamp);

			ImGui::TextDisabled("Tip: Ctrl+Click to input an exact value");

			ImGui_ButtonBar(apply_properties);

			ImGui::EndPopup();
		}
	}
}
