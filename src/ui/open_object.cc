#include "imgui.hh"
#include "main.hh"
#include "pkgman.hh"

namespace UiOpenObject {
	struct Entry {
		uint32_t id;
		std::string name;
		std::string modified;
	};

	static bool do_open = false;
	static bool multiemitter_mode = false;

	static std::vector<Entry> entries;

	static void refresh() {
		entries.clear();

		lvlfile *level = pkgman::get_levels(LEVEL_PARTIAL);

		while (level) {
			entries.push_back({
				level->id,
				level->name,
				level->modified_date,
			});

			lvlfile *next = level->next;
			delete level;
			level = next;
		}
	}

	static void confirm_import(uint32_t level_id) {
		if (multiemitter_mode)
			P.add_action(ACTION_MULTIEMITTER_SET, level_id);
		else
			P.add_action(ACTION_SELECT_IMPORT_OBJECT, level_id);

		ImGui::CloseCurrentPopup();
	}

	void open(bool _multiemitter_mode) {
		multiemitter_mode = _multiemitter_mode;
		do_open = true;

		refresh();
	}

	void layout() {
		handle_do_open(&do_open, "Import object###open_object");

		ImGui_CenterNextWindow();
		ImGui::SetNextWindowSize(UI(600.f, 0.f));

		if (ImGui::BeginPopupModal(multiemitter_mode ? "Select object###open_object" : "Import object###open_object", REF_TRUE, MODAL_FLAGS)) {

            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                ImGui::CloseCurrentPopup();

			ImGui::BeginChild("object_list_child", UI(0.f, 350.f), ImGuiChildFlags_NavFlattened, FRAME_FLAGS);

			if (ImGui::BeginTable("object_list", 4, ImGuiTableFlags_Borders)) {
				ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Last modified", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed);

				ImGui::TableHeadersRow();

				bool any_found = false;

				for (const Entry &entry : entries) {
					any_found = true;

					ImGui::PushID(entry.id);

					ImGui::TableNextRow();

					// ID
					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text("%u", entry.id);

					// Name
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(entry.name.c_str());

					// Modified
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(entry.modified.c_str());

					ImGui::TableNextColumn();

					if (ImGui::Button(multiemitter_mode ? "Select object" : "Import object"))
						confirm_import(entry.id);

					ImGui::PopID();
				}

				ImGui::EndTable();

				if (!any_found)
					ImGui::TextDisabled("No objects found.");
			}

			ImGui::EndChild();

			if (ImGui::IsKeyPressed(ImGuiKey_Escape))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}
}
