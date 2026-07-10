#include "entity.hh"
#include "game.hh"
#include "imgui.hh"
#include "main.hh"

namespace UiEmitter {
	static bool do_open = false;
	static float absorb_duration = 0.0f;

	void apply_properties() {
		entity* e = G->selection.e;
		if (e && (e->g_id == O_EMITTER || e->g_id == O_MINI_EMITTER)) {
			e->properties[6].v.f = absorb_duration;

			P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
			P.add_action(ACTION_RESELECT, 0);

			ImGui::CloseCurrentPopup();
		}
	}

	void open() {
		do_open = true;

		entity* e = G->selection.e;
		if (e && (e->g_id == O_EMITTER || e->g_id == O_MINI_EMITTER))
			absorb_duration = e->properties[6].v.f;
	}

	void layout() {
		handle_do_open(&do_open, "Emitter");

		ImGui_CenterNextWindow();
		if (ImGui::BeginPopupModal("Emitter", REF_TRUE, MODAL_FLAGS)) {

			ImGui::Text("Absorb entity after emitting:");

			ImGui::SliderFloat("##duration", &absorb_duration, 0.0f, 60.0f, "%.1f",
				ImGuiSliderFlags_AlwaysClamp);

			if (absorb_duration < 1.f)
				ImGui::Text("Don't absorb");
			else
				ImGui::Text("Duration: %.1f seconds", absorb_duration);

			ImGui::Dummy(ImVec2(0.0f, 5.0f));

			if (ImGui::Button("Save", ImVec2(80, 0)))
				apply_properties();

			ImGui::SameLine();

			if (ImGui::Button("Cancel", ImVec2(80, 0)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}
}
