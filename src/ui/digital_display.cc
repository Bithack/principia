#include "display.hh"
#include "entity.hh"
#include "game.hh"
#include "imgui.hh"

namespace UiDigitalDisplay {
    static bool do_open = false;

    static bool active_display = false;

    static uint64_t symbols[DISPLAY_MAX_SYMBOLS];
    static int num_symbols = 0;
    static int current_symbol = 0;

    static bool wrap = false;
    static int initial_position = 0;

    void apply_properties() {
        entity *e = G->selection.e;
        if (!e)
            return;

        e->properties[0].v.i8 = wrap;
        e->properties[1].v.i8 = initial_position - 1;

        char str[DISPLAY_MAX_SYMBOLS * 35 + 1];
        int ss = 0;

        for (int s = 0; s < num_symbols; ++s)
            for (int y = 0; y < 7; ++y)
                for (int x = 0; x < 5; ++x)
                    str[ss++] = (symbols[s] & (1ull << (y * 5 + x))) ? '1' : '0';

        str[ss] = '\0';

        e->set_property(2, str);
        ((display*)e)->load_symbols();

        ImGui::CloseCurrentPopup();
    }

    void open() {
        do_open = true;

        entity *e = G->selection.e;
        if (!e || (e->g_id != O_PASSIVE_DISPLAY && e->g_id != O_ACTIVE_DISPLAY))
            return;

        active_display = (e->g_id == O_ACTIVE_DISPLAY);

        display *d = (display*)e;

        num_symbols = d->num_symbols;
        current_symbol = d->properties[1].v.i8;

        memcpy(symbols, d->symbols, sizeof(symbols));

        wrap = d->properties[0].v.i8;
        initial_position = d->properties[1].v.i8 + 1;
    }

    void draw_pixel(int x, int y) {
        uint64_t bit = 1ull << (y * 5 + x);
        bool value = (symbols[current_symbol] & bit) != 0;

        ImGui::PushID(y * 5 + x);

        if (ImGui::Checkbox("##pixel", &value)) {
            if (value)
                symbols[current_symbol] |= bit;
            else
                symbols[current_symbol] &= ~bit;
        }

        ImGui::PopID();

        if (x != 4)
            ImGui::SameLine();
    }

    void insert_before() {
        if (num_symbols >= DISPLAY_MAX_SYMBOLS)
            return;

        memmove(
            &symbols[current_symbol + 1],
            &symbols[current_symbol],
            (num_symbols - current_symbol) *
                sizeof(uint64_t));

        symbols[current_symbol] = 0;
        num_symbols++;
    }

    void delete_current_symbol() {
        if (num_symbols <= 1)
            return;

        memmove(
            &symbols[current_symbol],
            &symbols[current_symbol + 1],
            (num_symbols - current_symbol - 1) *
                sizeof(uint64_t));

        num_symbols--;

        if (current_symbol >= num_symbols)
            current_symbol = num_symbols - 1;
    }

    void layout() {
        handle_do_open(&do_open, "Digital display###digi_display");

        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal(active_display ? "Active display###digi_display" : "Passive display###digi_display", REF_TRUE, MODAL_FLAGS)) {
            if (!active_display)
                ImGui::Checkbox("Wrap around", &wrap);

            ImGui::InputInt("Initial position", &initial_position);

            initial_position = tclampf(initial_position, 1, num_symbols);

            ImGui::Separator();

            ImGui::Text("Symbol %d / %d", current_symbol + 1, num_symbols);

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
            ImGui::PushStyleColor(ImGuiCol_CheckMark, IM_COL32(95, 189, 90, 255));
            ImGui::PushStyleColor(ImGuiCol_CheckboxSelectedBg, IM_COL32(95, 189, 90, 255));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(16, 16, 16, 255));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(80, 80, 80, 255));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(200, 200, 200, 255));

            for (int y = 0; y < 7; ++y)
                for (int x = 0; x < 5; ++x)
                    draw_pixel(x, y);

            ImGui::PopStyleColor(5);
            ImGui::PopStyleVar(2);

            ImGui::Spacing();

            if (ImGui::Button("Previous"))
                current_symbol--;

            ImGui::SameLine();

            if (ImGui::Button("Next"))
                current_symbol++;

            ImGui::Separator();

            current_symbol = tclampf(current_symbol, 0, num_symbols - 1);

            if (ImGui::Button("Insert before"))
                insert_before();

            ImGui::SameLine();

            if (ImGui::Button("Append"))
                if (num_symbols < DISPLAY_MAX_SYMBOLS)
                    symbols[num_symbols++] = 0;

            ImGui::SameLine();

            if (ImGui::Button("Delete current symbol"))
                delete_current_symbol();

            ImGui_ButtonBar(apply_properties);

            ImGui::EndPopup();
        }
    }
}
