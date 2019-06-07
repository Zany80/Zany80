#include <Zany80/API/graphics.h>
#include "stretchy_buffer.h"
#include <stdio.h>

#include "Zany80/internal/graphics.h"

#include <IMUI/IMUI.h>
#include <Gfx/Gfx.h>
using Oryol::Gfx;

static void render_widget(widget_t *widget) {
	if (!widget->visible) {
		return;
	}
    switch (widget->type) {
        case label:
            widget->_label.wrapped ? ImGui::TextWrapped("%s", widget->label) : ImGui::TextUnformatted(widget->label);
            break;
        case button:
            if (ImGui::Button(widget->label)) {
                if (widget->button.handler != NULL) {
                    widget->button.handler();
                }
            }
            break;
        case menu_item:
            if (ImGui::MenuItem(widget->label)) {
                if (widget->button.handler != NULL) {
                    widget->button.handler();
                }
            }
            break;
        case checkbox:
            if (ImGui::Checkbox(widget->label, widget->checkbox.value)) {
                if (widget->checkbox.handler != NULL) {
                    widget->checkbox.handler();
                }
            }
            break;
        case radio:
			if (ImGui::RadioButton(widget->label, widget->radio.current, widget->radio.index)) {
				widget->radio.handler(widget->radio.index);
			}
			break;
        case submenu:
			if (ImGui::BeginMenu(widget->menu->label)) {
				for (int j = 0; j < sb_count(widget->menu->widgets); j++) {
					render_widget(widget->menu->widgets[j]);
				}
				ImGui::EndMenu();
			}
			break;
        case group:
            for (int i = 0; i < sb_count(widget->_group.widgets); i++) {
                render_widget(widget->_group.widgets[i]);
                if (widget->_group.orientation == horizontal && i + 1 != sb_count(widget->_group.widgets)) {
                    ImGui::SameLine();
                }
            }
            break;
        case input:
			if (ImGui::InputText(widget->label, widget->input.buf, widget->input.capacity, ImGuiInputTextFlags_EnterReturnsTrue)) {
				widget->input.handler(widget);
			}
			break;
        case editor:
			widget->editor.editor->Render(widget->label);
			break;
        case custom:
            widget->custom.render();
            break;
    }
}

void render_window(window_t *window) {
    ImGuiWindowFlags flags = 0;
    if (sb_count(window->menus)) {
        flags |= ImGuiWindowFlags_MenuBar;
    }
    if (window == get_root()) {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(Gfx::DisplayAttrs().WindowWidth, Gfx::DisplayAttrs().WindowHeight));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0,0));
        flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
    }
    if (window->titlebar == false) {
        flags |= ImGuiWindowFlags_NoTitleBar;
    }
    if (window->minX != -1 || window->minY != -1 || window->maxX != -1 || window->maxY != -1) {
        float minX = window->minX == -1 ? 0 : window->minX;
        float minY = window->minY == -1 ? 0 : window->minY;
        float maxX = window->maxX == -1 ? FLT_MAX : window->maxX;
        float maxY = window->maxY == -1 ? FLT_MAX : window->maxY;
        ImGui::SetNextWindowSizeConstraints(ImVec2(minX, minY), ImVec2(maxX, maxY));
    }
    if (window->absX != -1 && window->absY != -1) {
        ImGui::SetNextWindowPos(ImVec2(window->absX, window->absY));
        window->absX = window->absY = -1;
    }
    bool minimized = true;
    if (ImGui::Begin(window->name, NULL, flags)) {
		minimized = false;
        if (window->initialX != -1 && window->initialY != -1) {
            ImGui::SetWindowSize(ImVec2(window->initialX, window->initialY), ImGuiCond_FirstUseEver);
        }
        if (window == get_root()) {
            ImGui::PopStyleVar(2);
        }
        if (sb_count(window->menus)) {
            if (ImGui::BeginMenuBar()) {
                for (int i = 0; i < sb_count(window->menus); i++) {
                    if (ImGui::BeginMenu(window->menus[i]->label)) {
                        for (int j = 0; j < sb_count(window->menus[i]->widgets); j++) {
                            render_widget(window->menus[i]->widgets[j]);
                        }
                        ImGui::EndMenu();
                    }
                }
                ImGui::EndMenuBar();
            }
        }
        for (int i = 0; i < sb_count(window->widgets); i++) {
            render_widget(window->widgets[i]);
        }
    }
    window->minimized = minimized;
    ImGui::End();
}

void window_get_pos(window_t *window, float *x, float *y) {
    if (ImGui::Begin(window->name)) {
        auto pos = ImGui::GetWindowPos();
        if (x)
            *x = pos.x;
        if (y)
            *y = pos.y;
    }
    ImGui::End();
}

void window_get_size(window_t *window, float *x, float *y) {
    if (ImGui::Begin(window->name)) {
        auto size = ImGui::GetWindowSize();
        if (x)
            *x = size.x;
        if (y)
            *y = size.y;
    }
    ImGui::End();
}

widget_t *editor_create(const char *label) {
	widget_t *w = widget_new(label);
	w->type = editor;
	w->editor.editor = new TextEditor();
	return w;
}

void editor_set_text(widget_t *widget, const char *text) {
	widget->editor.editor->SetText(text);
}

char *editor_get_text(widget_t *widget) {
	return strdup(widget->editor.editor->GetText().c_str());
}

void editor_destroy(TextEditor *editor) {
	delete editor;
}
