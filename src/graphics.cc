#include <stdio.h>

#include "SIMPLE/graphics.h"
#include "SIMPLE/3rd-party/stretchy_buffer.h"
#include "SIMPLE/internal/graphics.h"

#include <IMUI/IMUI.h>
#include <Gfx/Gfx.h>
#include <Gfx/private/gfxResourceContainer.h>
using Oryol::Gfx;
using Oryol::TextureSetup;
using Oryol::PixelFormat;
using Oryol::IMUI;

static widget_t **awaiting_handling = NULL;

typedef struct {
	Oryol::Id oryol;
	ImTextureID imgui;
} image_info_t;

static void render_widget(widget_t *widget) {
	if (!widget->visible) {
		return;
	}
    switch (widget->type) {
        case label:
            // Hack to avoid empty line when color change is the first part
            // of a label. TODO: avoid this.
            if (strlen(widget->label) != 0) {
                ImGui::PushStyleColor(ImGuiCol_Text, widget->_label.color);
                widget->_label.wrapped ? ImGui::TextWrapped("%s", widget->label) : ImGui::TextUnformatted(widget->label);
                ImGui::PopStyleColor();
            }
            if (widget->_label.next) {
                render_widget(widget->_label.next);
            }
            break;
        case button:
            if (ImGui::Button(widget->label)) {
                if (widget->button.handler != NULL) {
                    sb_push(awaiting_handling, widget);
                }
            }
            break;
        case menu_item:
            if (ImGui::MenuItem(widget->label)) {
                if (widget->button.handler != NULL) {
                    sb_push(awaiting_handling, widget);
                }
            }
            break;
        case checkbox:
            if (ImGui::Checkbox(widget->label, widget->checkbox.value)) {
                if (widget->checkbox.handler != NULL) {
                    sb_push(awaiting_handling, widget);
                }
            }
            break;
        case radio:
			if (ImGui::RadioButton(widget->label, widget->radio.current, widget->radio.index)) {
                if (widget->radio.handler != NULL) {
                    sb_push(awaiting_handling, widget);
                }
			}
			break;
        case submenu:
            if (widget->menu.collapsed) {
                if (ImGui::CollapsingHeader(widget->menu.menu->label)) {
                    for (int j = 0; j < sb_count(widget->menu.menu->widgets); j++) {
                        render_widget(widget->menu.menu->widgets[j]);
                    }
                }
            }
			else if (ImGui::BeginMenu(widget->menu.menu->label)) {
				for (int j = 0; j < sb_count(widget->menu.menu->widgets); j++) {
					render_widget(widget->menu.menu->widgets[j]);
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
            ImGui::TextUnformatted(widget->label);
            ImGui::SameLine();
			if (ImGui::InputText("", widget->input.buf, widget->input.capacity, ImGuiInputTextFlags_EnterReturnsTrue)) {
                if (widget->input.handler) {
                    sb_push(awaiting_handling, widget);
                }
			}
			break;
        case editor:
			widget->editor.editor->Render(widget->label);
			break;
		case image:
			if (widget->image.stream) {
				Oryol::ImageDataAttrs attrs;
				attrs.NumFaces = attrs.NumMipMaps = 1;
				attrs.Sizes[0][0] = widget->image.width * widget->image.height * 4;
				Gfx::UpdateTexture(((image_info_t*)widget->image.id)->oryol, widget->image.buf, attrs);
			}
			ImGui::Image(((image_info_t*)widget->image.id)->imgui, ImVec2(widget->image.visible_width, widget->image.visible_height));
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
        ImGui::SetNextWindowSize(ImVec2(Gfx::DisplayAttrs().FramebufferWidth, Gfx::DisplayAttrs().FramebufferHeight));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    }
    if (window->titlebar == false) {
        flags |= ImGuiWindowFlags_NoTitleBar;
    }
    if (window->auto_size) {
		flags |= ImGuiWindowFlags_AlwaysAutoResize;
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
    for (int i = 0; i < sb_count(awaiting_handling); i++) {
        widget_t *w = awaiting_handling[i];
        switch(w->type) {
            case button:
            case menu_item:
                w->button.handler();
                break;
            case checkbox:
                w->checkbox.handler();
                break;
            case radio:
                w->radio.handler(w->radio.index);
                break;
            case input:
                w->input.handler(w);
                break;
            default:
				break;
        }
    }
    if (window == get_root()) {
        ImGui::PopStyleVar();
    }
    sb_free(awaiting_handling);
    awaiting_handling = NULL;
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

widget_t *_image_create(uint8_t *buf, size_t width, size_t h, bool stream) {
	widget_t *w = widget_new(NULL);
	w->type = image;
	w->image.buf = buf;
	w->image.visible_width = w->image.width = width;
	w->image.visible_height = w->image.height = h;
	w->image.stream = false;
	TextureSetup tex_setup = TextureSetup::FromPixelData2D(width, h, 1, PixelFormat::RGBA8);
	tex_setup.ImageData.Sizes[0][0] = width * h * 4;
	w->image.id = new image_info_t;
	if ((w->image.stream = stream)) {
		tex_setup.TextureUsage = Oryol::Usage::Stream;
		((image_info_t*)w->image.id)->oryol = Gfx::CreateResource(tex_setup);
	}
	else {
		((image_info_t*)w->image.id)->oryol = Gfx::CreateResource(tex_setup, buf, width * h * 4);
	}
	((image_info_t*)w->image.id)->imgui = IMUI::AllocImage();
	IMUI::BindImage(((image_info_t*)w->image.id)->imgui, ((image_info_t*)w->image.id)->oryol);
	return w;
}

widget_t *image_create(uint8_t *buf, size_t width, size_t h) {
	return _image_create(buf, width, h, false);
}

widget_t *framebuffer_create(uint8_t *buf, size_t width, size_t h) {
	return _image_create(buf, width, h, true);
}

void image_free(widget_t *w) {
	IMUI::FreeImage(((image_info_t*)w->image.id)->imgui);
	free(w->image.id);
}
