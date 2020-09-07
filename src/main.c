#include <stdio.h>
#include <stdlib.h>
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_cimgui.h"

#define STR_(x) #x
#define STR(x) STR_(x)

void init(void) {
    printf("Zany80 version " STR(PROJECT_VERSION));
    sg_setup(&(sg_desc){
        .buffer_pool_size = 128,
    });
    scimgui_setup(&(scimgui_desc_t){
        .no_default_font = false,
    });
}

void frame(void) {
    int width = sapp_width();
    int height = sapp_height();
    scimgui_new_frame(width, height, 0.016);
    scimgui_render();
}

void deinit(void) {
    scimgui_shutdown();
    sg_shutdown();
}

void event(const sapp_event *event) {
    (void)event;
}

sapp_desc sokol_main(int argc, char **argv) {
    (void)argv;
    (void)argc;
    return (sapp_desc) {
        .width = 640,
        .height = 480,
        .window_title = "Zany80 v" STR(PROJECT_VERSION),
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = deinit,
        .event_cb = event,
        .sample_count = 4,
        .fullscreen = true,
        .alpha = true,
    };
}
