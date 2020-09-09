#include <stdio.h>
#include <string.h>
#include "TextEditor/TextEditor.h"
#define TEXT_EDITOR
#include "cimgui/imgui.h"
#include "stb/stb_ds.h"

extern "C" {
#include "graphics.h"
#include "sokol/sokol_app.h"
}

static widget_t **awaiting_handling = NULL;

static void render_widget(widget_t *widget) {
	if (!widget->visible) {
		return;
	}
	switch (widget->type) {
		case WIDGET_TYPE_LABEL:
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
		case WIDGET_TYPE_BUTTON:
			if (ImGui::Button(widget->label)) {
				if (widget->button.handler != NULL) {
					stbds_arrpush(awaiting_handling, widget);
				}
			}
			break;
		case WIDGET_TYPE_MENU_ITEM:
			if (ImGui::MenuItem(widget->label)) {
				if (widget->button.handler != NULL) {
					stbds_arrpush(awaiting_handling, widget);
				}
			}
			break;
		case WIDGET_TYPE_CHECKBOX:
			if (ImGui::Checkbox(widget->label, widget->checkbox.value)) {
				if (widget->checkbox.handler != NULL) {
					stbds_arrpush(awaiting_handling, widget);
				}
			}
			break;
		case WIDGET_TYPE_RADIO:{
			bool handler = false;
			if (widget->radio.current) {
				if (ImGui::RadioButton(widget->label, widget->radio.current, widget->radio.index)) {
					handler = true;
				}
			}
			else if (ImGui::RadioButton(widget->label, false)) {
				handler = true;
			}
			if (handler && widget->radio.handler != NULL) {
				stbds_arrpush(awaiting_handling, widget);
			}}
			break;
		case WIDGET_TYPE_SUBMENU:
			if (widget->menu.collapsed) {
				if (ImGui::CollapsingHeader(widget->menu.menu->label)) {
					for (size_t j = 0; j < stbds_arrlenu(widget->menu.menu->widgets); j++) {
						render_widget(widget->menu.menu->widgets[j]);
					}
				}
			}
			else if (ImGui::BeginMenu(widget->menu.menu->label)) {
				for (size_t j = 0; j < stbds_arrlenu(widget->menu.menu->widgets); j++) {
					render_widget(widget->menu.menu->widgets[j]);
				}
				ImGui::EndMenu();
			}
			break;
		case WIDGET_TYPE_GROUP:
			for (size_t i = 0; i < stbds_arrlenu(widget->_group.widgets); i++) {
				render_widget(widget->_group.widgets[i]);
				if (widget->_group.orientation == horizontal && i + 1 != stbds_arrlenu(widget->_group.widgets)) {
					ImGui::SameLine();
				}
			}
			break;
		case WIDGET_TYPE_INPUT:{
			ImGui::TextUnformatted(widget->label);
			ImGui::SameLine();
			int flags = ImGuiInputTextFlags_EnterReturnsTrue;
			if (widget->input.pw) {
				flags |= ImGuiInputTextFlags_Password;
			}
			if (ImGui::InputText("", widget->input.buf, widget->input.capacity, flags)) {
				if (widget->input.handler) {
					stbds_arrpush(awaiting_handling, widget);
				}
			}
			break;}
		case WIDGET_TYPE_EDITOR:
			widget->editor.editor->Render(widget->label);
			break;
		case WIDGET_TYPE_IMAGE:
			if (widget->image.stream) {
				//Oryol::ImageDataAttrs attrs;
				//attrs.NumFaces = attrs.NumMipMaps = 1;
				//attrs.Sizes[0][0] = widget->image.width * widget->image.height * 4;
				//Gfx::UpdateTexture(((image_info_t*)widget->image.id)->oryol, widget->image.buf, attrs);

			}
//			ImGui::Image(((image_info_t*)widget->image.id)->imgui, ImVec2(widget->image.visible_width, widget->image.visible_height));
			break;
		case WIDGET_TYPE_CUSTOM:
			widget->custom.render();
			break;
	}
}

void render_window(window_t *window) {
	ImGuiWindowFlags flags = 0;
	if (stbds_arrlenu(window->menus)) {
		flags |= ImGuiWindowFlags_MenuBar;
	}
	if (window == get_root()) {
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(ImVec2(sapp_width(), sapp_height()));
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
		if (stbds_arrlenu(window->menus)) {
			if (ImGui::BeginMenuBar()) {
				for (size_t i = 0; i < stbds_arrlenu(window->menus); i++) {
					if (ImGui::BeginMenu(window->menus[i]->label)) {
						for (size_t j = 0; j < stbds_arrlenu(window->menus[i]->widgets); j++) {
							render_widget(window->menus[i]->widgets[j]);
						}
						ImGui::EndMenu();
					}
				}
				ImGui::EndMenuBar();
			}
		}
		for (size_t i = 0; i < stbds_arrlenu(window->widgets); i++) {
			render_widget(window->widgets[i]);
		}
	}
	window->minimized = minimized;
	ImGui::End();
	if (window == get_root()) {
		ImGui::PopStyleVar();
	}
	for (size_t i = 0; i < stbds_arrlenu(awaiting_handling); i++) {
		widget_t *w = awaiting_handling[i];
		switch(w->type) {
			case WIDGET_TYPE_BUTTON:
			case WIDGET_TYPE_MENU_ITEM:
				w->button.handler();
				break;
			case WIDGET_TYPE_CHECKBOX:
				w->checkbox.handler();
				break;
			case WIDGET_TYPE_RADIO:
				w->radio.handler(w->radio.index);
				break;
			case WIDGET_TYPE_INPUT:
				w->input.handler(w);
				break;
			default:
				break;
		}
	}
	stbds_arrfree(awaiting_handling);
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
	w->type = WIDGET_TYPE_EDITOR;
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
	return NULL;
	widget_t *w = widget_new(NULL);
	w->type = WIDGET_TYPE_IMAGE;
	w->image.buf = buf;
	w->image.visible_width = w->image.width = width;
	w->image.visible_height = w->image.height = h;
	w->image.stream = false;
	//TextureSetup tex_setup = TextureSetup::FromPixelData2D(width, h, 1, PixelFormat::RGBA8);
	//tex_setup.ImageData.Sizes[0][0] = width * h * 4;
	//w->image.id = new image_info_t;
	//if ((w->image.stream = stream)) {
	//	tex_setup.TextureUsage = Oryol::Usage::Stream;
	//	((image_info_t*)w->image.id)->oryol = Gfx::CreateResource(tex_setup);
	//}
	//else {
	//	((image_info_t*)w->image.id)->oryol = Gfx::CreateResource(tex_setup, buf, width * h * 4);
	//}
	//((image_info_t*)w->image.id)->imgui = IMUI::AllocImage();
	//IMUI::BindImage(((image_info_t*)w->image.id)->imgui, ((image_info_t*)w->image.id)->oryol);
	return w;
}

widget_t *image_create(uint8_t *buf, size_t width, size_t h) {
	return _image_create(buf, width, h, false);
}

widget_t *framebuffer_create(uint8_t *buf, size_t width, size_t h) {
	return _image_create(buf, width, h, true);
}

void image_free(widget_t *w) {
	//IMUI::FreeImage(((image_info_t*)w->image.id)->imgui);
	//free(w->image.id);
}
